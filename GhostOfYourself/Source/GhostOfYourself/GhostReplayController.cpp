#include "GhostReplayController.h"
#include "RecordedGhost.h"
#include "GhostButton.h"
#include "GhostButtonGroup.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/TargetPoint.h"
#include "Components/CapsuleComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogGhostReplay, Log, All);

AGhostReplayController::AGhostReplayController()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    GhostClass = ARecordedGhost::StaticClass();
}

bool AGhostReplayController::TryInitializePlayer()
{
    ControllerRef = UGameplayStatics::GetPlayerController(this, 0);
    PlayerRef = ControllerRef ? Cast<ACharacter>(ControllerRef->GetPawn()) : nullptr;
    if (!IsValid(PlayerRef) || !IsValid(ControllerRef) || !ControllerRef->IsLocalController())
    {
        return false;
    }
    StartTransform = PlayerRef->GetActorTransform();
    StartControlRotation = ControllerRef->GetControlRotation();
    AddTickPrerequisiteActor(ControllerRef);
    AddTickPrerequisiteActor(PlayerRef);
    AddTickPrerequisiteComponent(PlayerRef->GetCharacterMovement());
    AddTickPrerequisiteComponent(PlayerRef->GetMesh());
    bInitialized = true;
    Notify(TEXT("C++ ghost ready: R record/save | X cancel/restart | Z remove | E button"));
    return true;
}

void AGhostReplayController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bInitialized && !TryInitializePlayer())
    {
        return; // Pawn possession can happen after actor BeginPlay.
    }
    if (!IsValid(PlayerRef) || !IsValid(ControllerRef))
    {
        if (bResetInProgress)
        {
            ReleaseResetInput();
            bResetInProgress = false;
            bRecordAfterReset = false;
        }
        return;
    }

    if (bResetInProgress)
    {
        UpdateResetTransition(DeltaSeconds);
        ShowStatus();
        return;
    }

    // Timestamp this frame's movement before handling a manual save.
    // A reset/start below sets the shared clock back to zero.
    if (bRecording)
    {
        Clock = FMath::Min(Clock + DeltaSeconds, FMath::Max(0.05f, MaxRecordingSeconds));
    }
    else
    {
        Clock += DeltaSeconds;
        if (IsValid(GhostRef))
        {
            GhostRef->EvaluateAtTime(Clock);
            ReplayButtonEvents();
        }
    }

    // Independent of the unfinished Blueprint IMC and recording event graph.
    // These are edge checks: holding R does not repeatedly toggle the state.
    if (ControllerRef->WasInputKeyJustPressed(EKeys::Z))
    {
        UndoGhost();
    }
    else if (ControllerRef->WasInputKeyJustPressed(EKeys::X))
    {
        CancelOrRestart();
    }
    else if (ControllerRef->WasInputKeyJustPressed(EKeys::R))
    {
        ToggleRecording();
    }
    else if (bRecording)
    {
        if (Clock >= FMath::Max(0.05f, MaxRecordingSeconds))
        {
            CommitRecording();
        }
        else if (Clock >= NextSampleTime)
        {
            CaptureSample();
        }
    }
    if (!bResetInProgress && ControllerRef->WasInputKeyJustPressed(EKeys::E))
    {
        if (!TryPressButton(FindButtonToPress()))
        {
            Notify(TEXT("Look at a nearby active button and press E."));
        }
    }
    ShowStatus();
}

AGhostButton* AGhostReplayController::FindButtonToPress() const
{
    if (!IsValid(PlayerRef) || !IsValid(ControllerRef)) { return nullptr; }
    FVector Eye;
    FRotator View;
    PlayerRef->GetActorEyesViewPoint(Eye, View);
    AGhostButton* Best = nullptr;
    float BestDot = .75f;
    for (TActorIterator<AGhostButton> It(GetWorld()); It; ++It)
    {
        AGhostButton* Button = *It;
        if (!IsValid(Button->GroupRef) || Button->GroupRef->ControllerRef != this
            || !Button->GroupRef->IsActiveRoom() || Button->GroupRef->bUnlocked
            || !Button->IsInRange(PlayerRef->GetActorLocation())) { continue; }
        const FVector Target = Button->ButtonMesh->GetComponentLocation();
        const float Dot = FVector::DotProduct(View.Vector(), (Target - Eye).GetSafeNormal());
        if (Dot < BestDot) { continue; }
        FCollisionQueryParams Query(SCENE_QUERY_STAT(GhostButtonInteraction), true, PlayerRef);
        if (IsValid(GhostRef)) { Query.AddIgnoredActor(GhostRef); }
        FHitResult Hit;
        if (GetWorld()->LineTraceSingleByChannel(Hit, Eye, Target, ECC_Visibility, Query)
            && Hit.GetActor() != Button) { continue; }
        BestDot = Dot;
        Best = Button;
    }
    return Best;
}

bool AGhostReplayController::TryPressButton(AGhostButton* Button)
{
    if (!bInitialized || bResetInProgress || !IsValid(PlayerRef) || !IsValid(Button)
        || !IsValid(Button->GroupRef) || Button->GroupRef->ControllerRef != this) { return false; }
    if (!Button->GroupRef->ReceivePress(Button, Clock, PlayerRef->GetActorLocation())) { return false; }
    if (bRecording)
    {
        // A sample at the exact event time keeps range validation independent of the 20 Hz cadence.
        CaptureSample();
        FRecordedGhostButtonPress Press;
        Press.Time = Clock;
        Press.Button = Button;
        WorkingButtonPresses.Add(Press);
    }
    Notify(FString::Printf(TEXT("Button pressed at %.2fs."), Clock));
    return true;
}

void AGhostReplayController::ReplayButtonEvents()
{
    while (NextButtonEvent < SavedButtonPresses.Num() && SavedButtonPresses[NextButtonEvent].Time <= Clock)
    {
        const FRecordedGhostButtonPress& Press = SavedButtonPresses[NextButtonEvent++];
        AGhostButton* Button = Press.Button.Get();
        if (!IsValid(Button) || !IsValid(Button->GroupRef) || Button->GroupRef->ControllerRef != this
            || SavedSamples.IsEmpty() || Press.Time < 0.f || Press.Time > SavedSamples.Last().Time) { continue; }
        int32 Cursor = 0;
        FTransform RecordedPose;
        if (GhostRecording::Evaluate(SavedSamples, Press.Time, Cursor, RecordedPose))
        {
            // Check location at the event time, not the ghost's later location after a slow frame.
            Button->GroupRef->ReceivePress(Button, Press.Time, RecordedPose.GetLocation());
        }
    }
}

void AGhostReplayController::ToggleRecording()
{
    if (!bInitialized || bResetInProgress)
    {
        return;
    }
    if (bRecording)
    {
        CommitRecording();
        return;
    }
    if (!SavedSamples.IsEmpty())
    {
        Notify(TEXT("Echo limit reached (1/1). Z removes it; X restarts."));
        return;
    }
    WorkingSamples.Reset();
    WorkingButtonPresses.Reset();
    ResetAttempt(true);
}

void AGhostReplayController::CaptureSample()
{
    FRecordedGhostSample Sample;
    Sample.Time = Clock;
    Sample.Transform = PlayerRef->GetActorTransform();
    if (USkeletalMeshComponent* Body = PlayerRef->GetMesh())
    {
        Sample.BodyTransform = Body->GetRelativeTransform();
        Sample.BoneTransforms = Body->GetBoneSpaceTransforms();
    }
    // Replace, rather than append, a same-time endpoint from an immediate save.
    if (!WorkingSamples.IsEmpty() && Sample.Time <= WorkingSamples.Last().Time)
    {
        WorkingSamples.Last() = Sample;
    }
    else
    {
        WorkingSamples.Add(Sample);
    }
    // Keep the cadence on the shared clock instead of accumulating frame overshoot.
    // After a slow frame, take one real sample and skip missed slots.
    const float Interval = FMath::Max(0.01f, SampleInterval);
    NextSampleTime = (FMath::FloorToFloat(Clock / Interval) + 1.f) * Interval;
}

void AGhostReplayController::CommitRecording()
{
    if (!bRecording || !IsValid(PlayerRef))
    {
        return;
    }
    CaptureSample();
    bRecording = false;
    SavedSamples = MoveTemp(WorkingSamples);
    SavedButtonPresses = MoveTemp(WorkingButtonPresses);

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    GhostRef = GetWorld()->SpawnActor<ARecordedGhost>(
        GhostClass ? GhostClass.Get() : ARecordedGhost::StaticClass(),
        SavedSamples[0].Transform, Params);
    if (!IsValid(GhostRef))
    {
        SavedSamples.Reset();
        SavedButtonPresses.Reset();
        ResetAttempt();
        Notify(TEXT("Ghost spawn failed; recording slot is free. See Output Log."));
        return;
    }
    GhostRef->SetEchoNumber(1); // One slot today; future echoes use 2 (yellow), 3 (green).
    GhostRef->ConfigureFromPlayer(PlayerRef);
    GhostRef->SetRecording(SavedSamples);
    const int32 Count = SavedSamples.Num();
    const float Duration = SavedSamples.Last().Time;
    ResetAttempt();
    Notify(FString::Printf(TEXT("Saved %d samples (%.2fs). Echo replaying."), Count, Duration));
}

void AGhostReplayController::ResetAttempt(bool bStartRecording)
{
    if (bResetInProgress)
    {
        return;
    }
    bRecordAfterReset = bStartRecording;
    bRecording = false;
    APlayerCameraManager* Camera = IsValid(ControllerRef) ? ControllerRef->PlayerCameraManager : nullptr;
    if (!bUseResetFade || !IsValid(Camera)
        || (FadeOutSeconds <= UE_SMALL_NUMBER && FadeInSeconds <= UE_SMALL_NUMBER))
    {
        PerformReset();
        FinishResetTransition();
        return;
    }
    bResetInProgress = true;
    bFadingIn = false;
    FadeElapsed = 0.f;
    ControllerRef->SetIgnoreMoveInput(true);
    ControllerRef->SetIgnoreLookInput(true);
    bResetInputLocked = true;
    PlayerRef->GetCharacterMovement()->StopMovementImmediately();
    PlayerRef->StopJumping();
    Camera->SetManualCameraFade(0.f, FLinearColor::Black, false);
}

void AGhostReplayController::UpdateResetTransition(float DeltaSeconds)
{
    PlayerRef->GetCharacterMovement()->StopMovementImmediately();
    PlayerRef->StopJumping();
    FadeElapsed += FMath::Max(0.f, DeltaSeconds);
    APlayerCameraManager* Camera = ControllerRef->PlayerCameraManager;
    const float Duration = FMath::Max(0.f, bFadingIn ? FadeInSeconds : FadeOutSeconds);
    const float Alpha = Duration > UE_SMALL_NUMBER ? FMath::Clamp(FadeElapsed / Duration, 0.f, 1.f) : 1.f;
    if (IsValid(Camera))
    {
        Camera->SetManualCameraFade(bFadingIn ? 1.f - Alpha : Alpha, FLinearColor::Black, false);
    }
    if (Alpha < 1.f)
    {
        return;
    }
    if (!bFadingIn)
    {
        // Keep this frame fully black; fade-in starts on the following tick.
        PerformReset();
        bFadingIn = true;
        FadeElapsed = 0.f;
    }
    else
    {
        FinishResetTransition();
    }
}

void AGhostReplayController::ReleaseResetInput()
{
    if (IsValid(ControllerRef))
    {
        if (bResetInputLocked)
        {
            ControllerRef->SetIgnoreMoveInput(false);
            ControllerRef->SetIgnoreLookInput(false);
        }
        if (IsValid(ControllerRef->PlayerCameraManager))
        {
            ControllerRef->PlayerCameraManager->StopCameraFade();
        }
    }
    bResetInputLocked = false;
}

void AGhostReplayController::FinishResetTransition()
{
    ReleaseResetInput();
    bResetInProgress = false;
    if (bRecordAfterReset && IsValid(PlayerRef))
    {
        CaptureSample();
        bRecording = true;
        Notify(TEXT("Recording started. R saves; X cancels."));
    }
    bRecordAfterReset = false;
}

void AGhostReplayController::PerformReset()
{
    bRecording = false;
    Clock = 0.f;
    NextSampleTime = 0.f;
    NextButtonEvent = 0;
    if (IsValid(PlayerRef) && IsValid(ControllerRef))
    {
        PlayerRef->GetCharacterMovement()->StopMovementImmediately();
        PlayerRef->SetActorLocationAndRotation(StartTransform.GetLocation(),
            StartTransform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
        ControllerRef->SetControlRotation(StartControlRotation);
    }
    if (IsValid(GhostRef))
    {
        GhostRef->EvaluateAtTime(0.f);
    }
    OnAttemptReset.Broadcast();
}

void AGhostReplayController::CancelOrRestart()
{
    if (!bInitialized || bResetInProgress)
    {
        return;
    }
    const bool bWasRecording = bRecording;
    WorkingSamples.Reset();
    WorkingButtonPresses.Reset();
    ResetAttempt();
    Notify(bWasRecording ? TEXT("Recording canceled.") : TEXT("Attempt restarted."));
}

void AGhostReplayController::UndoGhost()
{
    if (!bInitialized || bResetInProgress)
    {
        return;
    }
    WorkingSamples.Reset();
    WorkingButtonPresses.Reset();
    SavedSamples.Reset();
    SavedButtonPresses.Reset();
    if (IsValid(GhostRef))
    {
        GhostRef->Destroy();
    }
    GhostRef = nullptr;
    ResetAttempt();
    Notify(TEXT("Echo removed. R starts a new recording."));
}

void AGhostReplayController::ShowStatus() const
{
    if (bShowDebugStatus && GEngine)
    {
        const TCHAR* State = bResetInProgress ? TEXT("Resetting") : bRecording ? TEXT("Recording")
            : (!SavedSamples.IsEmpty() ? TEXT("Replaying") : TEXT("Exploring"));
        const FString Status = FString::Printf(
            TEXT("Room %d | %s | %.2fs | Echo %d/1 | R record/save | X cancel/restart | Z remove | E button"),
            CurrentRoomNumber, State, Clock, SavedSamples.IsEmpty() ? 0 : 1);
        GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.f,
            bRecording ? FColor::Red : FColor::Cyan, Status);
    }
}

bool AGhostReplayController::AdvanceToNextRoom(const ATargetPoint* NextRoomStart)
{
    if (!bInitialized || bRecording || bResetInProgress || !IsValid(PlayerRef) || !IsValid(ControllerRef))
    {
        return false;
    }
    StartTransform = PlayerRef->GetActorTransform();
    StartControlRotation = ControllerRef->GetControlRotation();
    if (IsValid(NextRoomStart))
    {
        // The marker is floor-level; character actor origins are capsule centers.
        StartTransform.SetLocation(NextRoomStart->GetActorLocation()
            + FVector(0.f, 0.f, PlayerRef->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
        StartTransform.SetRotation(FRotator(0.f, NextRoomStart->GetActorRotation().Yaw, 0.f).Quaternion());
        StartControlRotation = FRotator(0.f, NextRoomStart->GetActorRotation().Yaw, 0.f);
    }
    WorkingSamples.Reset();
    WorkingButtonPresses.Reset();
    SavedSamples.Reset();
    SavedButtonPresses.Reset();
    if (IsValid(GhostRef))
    {
        GhostRef->Destroy();
    }
    GhostRef = nullptr;
    Clock = 0.f;
    NextSampleTime = 0.f;
    NextButtonEvent = 0;
    ++CurrentRoomNumber;
    OnAttemptReset.Broadcast();
    Notify(FString::Printf(TEXT("Room %d ready. Echo cleared; R starts a new recording."), CurrentRoomNumber));
    return true;
}

void AGhostReplayController::Notify(const FString& Message) const
{
    UE_LOG(LogGhostReplay, Display, TEXT("%s"), *Message);
    if (GEngine && bShowDebugStatus)
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Cyan, Message);
    }
}

void AGhostReplayController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ReleaseResetInput();
    if (IsValid(GhostRef))
    {
        GhostRef->Destroy();
    }
    if (GEngine)
    {
        GEngine->RemoveOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()));
    }
    Super::EndPlay(EndPlayReason);
}
