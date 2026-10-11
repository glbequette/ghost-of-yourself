#include "GhostReplayController.h"
#include "GhostPauseMenu.h"
#include "RecordedGhost.h"
#include "GhostButton.h"
#include "GhostButtonGroup.h"
#include "GhostExitZone.h"
#include "GhostIntroController.h"
#include "GhostGate.h"
#include "GhostRoomNarration.h"
#include "GhostLabMusic.h"
#include "GameFramework/HUD.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
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
    RoomGhostLimits.Add(6, 2); // Preserve the current Room 6 setup; other rooms default to one.
    RoomNarration = CreateDefaultSubobject<UGhostRoomNarration>(TEXT("RoomNarration"));
    LabMusic = CreateDefaultSubobject<UGhostLabMusic>(TEXT("LabMusic"));
    PauseMenu = CreateDefaultSubobject<UGhostPauseMenu>(TEXT("PauseMenu"));
}

int32 AGhostReplayController::GetGhostLimit() const
{
    if (CurrentRoomNumber == 0) { return 0; } // The welcome room does not record.
    const int32* RoomLimit = RoomGhostLimits.Find(CurrentRoomNumber);
    return FMath::Max(0, RoomLimit ? *RoomLimit : DefaultGhostLimit);
}

int32 AGhostReplayController::GetSavedGhostCount() const
{
    return (SavedSamples.IsEmpty() ? 0 : 1) + AdditionalGhosts.Num();
}

void AGhostReplayController::GetGhostActors(TArray<ARecordedGhost*>& OutGhosts, bool bIncludeSuppressed) const
{
    OutGhosts.Reset();
    if (IsValid(GhostRef) && (bIncludeSuppressed || GhostRef.Get() != SuppressedGhost.Get())) { OutGhosts.Add(GhostRef.Get()); }
    for (const FAdditionalGhostReplay& Track : AdditionalGhosts)
    {
        if (IsValid(Track.Ghost) && (bIncludeSuppressed || Track.Ghost.Get() != SuppressedGhost.Get())) { OutGhosts.Add(Track.Ghost.Get()); }
    }
}

void AGhostReplayController::ClearSavedGhosts()
{
    RestoreSuppressedGhost();
    TArray<ARecordedGhost*> Ghosts;
    GetGhostActors(Ghosts);
    for (ARecordedGhost* Ghost : Ghosts) { Ghost->Destroy(); }
    GhostRef = nullptr;
    SavedSamples.Reset();
    SavedButtonPresses.Reset();
    AdditionalGhosts.Reset();
    NextButtonEvent = 0;
}

void AGhostReplayController::RemoveOldestGhost()
{
    if (IsValid(GhostRef)) { GhostRef->Destroy(); }
    GhostRef = nullptr;
    SavedSamples.Reset();
    SavedButtonPresses.Reset();
    NextButtonEvent = 0;
    if (!AdditionalGhosts.IsEmpty())
    {
        FAdditionalGhostReplay OldestRemaining = MoveTemp(AdditionalGhosts[0]);
        AdditionalGhosts.RemoveAt(0);
        GhostRef = OldestRemaining.Ghost;
        SavedSamples = MoveTemp(OldestRemaining.Samples);
        SavedButtonPresses = MoveTemp(OldestRemaining.ButtonPresses);
    }
}

int32 AGhostReplayController::FindFreeEchoNumber() const
{
    TArray<ARecordedGhost*> Ghosts;
    GetGhostActors(Ghosts, true);
    TSet<int32> UsedNumbers;
    for (ARecordedGhost* Ghost : Ghosts) { UsedNumbers.Add(Ghost->EchoNumber); }
    int32 Number = 1;
    while (UsedNumbers.Contains(Number)) { ++Number; }
    return Number;
}

void AGhostReplayController::RestoreSuppressedGhost()
{
    if (ARecordedGhost* Ghost = SuppressedGhost.Get()) { Ghost->SetActorHiddenInGame(bSuppressedGhostWasHidden); }
    SuppressedGhost.Reset();
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
    PlayerRef->GetCharacterMovement()->MaxWalkSpeed = FMath::Max(0.f, PlayerMaxWalkSpeed);
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
    if (UGameplayStatics::IsGamePaused(this)) { return; }
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

    if (bGameCompleted)
    {
        // Narration and music components continue ticking on their own clocks.
        if (IsCompletionNarrationFinished() && !bTitleTravelRequested)
        {
            if (!bCompletionFading)
            {
                bCompletionFading = true;
                if (ControllerRef->PlayerCameraManager)
                {
                    ControllerRef->PlayerCameraManager->StartCameraFade(0.f, 1.f, .5f, FLinearColor::Black, true, true);
                }
            }
            CompletionElapsed += FMath::Max(0.f, DeltaSeconds);
            if (CompletionElapsed >= FMath::Max(.5f, CompletionReturnDelay))
            {
                bTitleTravelRequested = true;
                UGameplayStatics::OpenLevel(this, CompletionTitleLevel);
            }
        }
        return;
    }
    UpdateHUDRegistration();
    // Keep the same walking cap for recording and live play, including room resets.
    PlayerRef->GetCharacterMovement()->MaxWalkSpeed = FMath::Max(0.f, PlayerMaxWalkSpeed);
    WispAnimationTime += FMath::Max(0.f, DeltaSeconds);
    RecordingWispBlend = FMath::FInterpTo(RecordingWispBlend,
        bRecording && !bResetInProgress ? 1.f : 0.f, FMath::Max(0.f, DeltaSeconds), 7.f);

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
    }
    // Existing echoes keep moving and pressing buttons while the next echo is being recorded.
    TArray<ARecordedGhost*> Ghosts;
    GetGhostActors(Ghosts);
    for (ARecordedGhost* Ghost : Ghosts) { Ghost->EvaluateAtTime(Clock); }
    ReplayButtonEvents();

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
            || !Button->GroupRef->IsActiveRoom() || Button->GroupRef->bUnlocked || Button->GroupRef->IsButtonUsed(Button)
            || !Button->IsInRange(PlayerRef->GetActorLocation())) { continue; }
        const FVector Target = Button->ButtonMesh->GetComponentLocation();
        const float Dot = FVector::DotProduct(View.Vector(), (Target - Eye).GetSafeNormal());
        if (Dot < BestDot) { continue; }
        FCollisionQueryParams Query(SCENE_QUERY_STAT(GhostButtonInteraction), true, PlayerRef);
        TArray<ARecordedGhost*> Ghosts;
        GetGhostActors(Ghosts);
        for (ARecordedGhost* Ghost : Ghosts) { Query.AddIgnoredActor(Ghost); }
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
    if (bGameCompleted) { return false; }
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
    // Merge events from all tracks in time order, including after a slow frame.
    // Processing one whole track first could expire the other track's earlier press incorrectly.
    while (true)
    {
        const TArray<FRecordedGhostButtonPress>* Events = &SavedButtonPresses;
        const TArray<FRecordedGhostSample>* Samples = &SavedSamples;
        ARecordedGhost* EventGhost = GhostRef.Get();
        int32* Next = &NextButtonEvent;
        float Earliest = NextButtonEvent < SavedButtonPresses.Num()
            ? SavedButtonPresses[NextButtonEvent].Time : TNumericLimits<float>::Max();
        for (FAdditionalGhostReplay& Track : AdditionalGhosts)
        {
            if (Track.NextButtonEvent < Track.ButtonPresses.Num()
                && Track.ButtonPresses[Track.NextButtonEvent].Time < Earliest)
            {
                Earliest = Track.ButtonPresses[Track.NextButtonEvent].Time;
                Events = &Track.ButtonPresses;
                Samples = &Track.Samples;
                EventGhost = Track.Ghost.Get();
                Next = &Track.NextButtonEvent;
            }
        }
        if (*Next >= Events->Num() || Earliest > Clock) { break; }
        const FRecordedGhostButtonPress& Press = (*Events)[(*Next)++];
        AGhostButton* Button = Press.Button.Get();
        if (!IsValid(EventGhost) || EventGhost == SuppressedGhost.Get() || !IsValid(Button) || !IsValid(Button->GroupRef) || Button->GroupRef->ControllerRef != this
            || Samples->IsEmpty() || Press.Time < 0.f || Press.Time > Samples->Last().Time) { continue; }
        int32 Cursor = 0;
        FTransform RecordedPose;
        if (GhostRecording::Evaluate(*Samples, Press.Time, Cursor, RecordedPose))
        {
            // Check location at the event time, not the ghost's later location after a slow frame.
            Button->GroupRef->ReceivePress(Button, Press.Time, RecordedPose.GetLocation());
        }
    }
}

void AGhostReplayController::ToggleRecording()
{
    if (bGameCompleted) { return; }
    if (CurrentRoomNumber == 0)
    {
        Notify(TEXT("Welcome. Recording begins in Room 1."));
        return;
    }
    if (!bInitialized || bResetInProgress)
    {
        return;
    }
    if (bRecording)
    {
        CommitRecording();
        return;
    }
    if (GetGhostLimit() == 0)
    {
        Notify(TEXT("Recording is disabled in this room."));
        return;
    }
    if (GetSavedGhostCount() >= GetGhostLimit())
    {
        Notify(TEXT("The next saved recording will replace the oldest echo. X cancels without replacing it."));
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
    if (GetGhostLimit() == 0)
    {
        WorkingSamples.Reset();
        WorkingButtonPresses.Reset();
        ResetAttempt();
        Notify(TEXT("Recording is disabled; existing recordings preserved."));
        return;
    }
    CaptureSample();
    bRecording = false;

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ARecordedGhost* NewGhost = GetWorld()->SpawnActor<ARecordedGhost>(
        GhostClass ? GhostClass.Get() : ARecordedGhost::StaticClass(),
        WorkingSamples[0].Transform, Params);
    if (!IsValid(NewGhost))
    {
        WorkingSamples.Reset();
        WorkingButtonPresses.Reset();
        ResetAttempt();
        Notify(TEXT("Ghost spawn failed; existing recordings preserved. See Output Log."));
        return;
    }
    // Retire old tracks only after the new actor exists. Cancel/spawn failure keeps all old tracks.
    while (GetSavedGhostCount() >= GetGhostLimit()) { RemoveOldestGhost(); }
    NewGhost->SetEchoNumber(FindFreeEchoNumber());
    NewGhost->ConfigureFromPlayer(PlayerRef);
    NewGhost->SetRecording(WorkingSamples);
    const int32 Count = WorkingSamples.Num();
    const float Duration = WorkingSamples.Last().Time;
    if (SavedSamples.IsEmpty())
    {
        GhostRef = NewGhost;
        SavedSamples = MoveTemp(WorkingSamples);
        SavedButtonPresses = MoveTemp(WorkingButtonPresses);
    }
    else
    {
        FAdditionalGhostReplay& Track = AdditionalGhosts.AddDefaulted_GetRef();
        Track.Ghost = NewGhost;
        Track.Samples = MoveTemp(WorkingSamples);
        Track.ButtonPresses = MoveTemp(WorkingButtonPresses);
    }
    ResetAttempt();
    Notify(FString::Printf(TEXT("Saved %d samples (%.2fs). Echo replaying."), Count, Duration));
}

void AGhostReplayController::ResetAttempt(bool bStartRecording)
{
    if (bResetInProgress)
    {
        return;
    }
    RestoreSuppressedGhost();
    if (bStartRecording && GetGhostLimit() > 0 && GetSavedGhostCount() >= GetGhostLimit() && IsValid(GhostRef))
    {
        SuppressedGhost = GhostRef.Get();
        bSuppressedGhostWasHidden = GhostRef->IsHidden();
        GhostRef->SetActorHiddenInGame(true);
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
    for (FAdditionalGhostReplay& Track : AdditionalGhosts)
    {
        Track.NextButtonEvent = 0;
        if (IsValid(Track.Ghost)) { Track.Ghost->EvaluateAtTime(0.f); }
    }
    OnAttemptReset.Broadcast();
}

void AGhostReplayController::CancelOrRestart()
{
    if (bGameCompleted) { return; }
    if (CurrentRoomNumber == 0) { return; }
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
    if (bGameCompleted) { return; }
    if (CurrentRoomNumber == 0) { return; }
    if (!bInitialized || bResetInProgress)
    {
        return;
    }
    WorkingSamples.Reset();
    WorkingButtonPresses.Reset();
    if (!AdditionalGhosts.IsEmpty())
    {
        if (IsValid(AdditionalGhosts.Last().Ghost)) { AdditionalGhosts.Last().Ghost->Destroy(); }
        AdditionalGhosts.Pop();
    }
    else { ClearSavedGhosts(); }
    ResetAttempt();
    Notify(TEXT("Newest echo removed. R records another; X restarts."));
}

void AGhostReplayController::ShowStatus() const
{
    if (bShowDebugStatus && GEngine)
    {
        const TCHAR* State = bResetInProgress ? TEXT("Resetting") : bRecording ? TEXT("Recording")
            : (!SavedSamples.IsEmpty() ? TEXT("Replaying") : TEXT("Exploring"));
        const FString Status = FString::Printf(
            TEXT("Room %d | %s | %.2fs | Echo %d/%d | R record/save | X cancel/restart | Z remove newest | E button"),
            CurrentRoomNumber, State, Clock, GetSavedGhostCount(), GetGhostLimit());
        GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.f,
            bRecording ? FColor::Red : FColor::Cyan, Status);
    }
}

bool AGhostReplayController::JumpToRoom(int32 RoomNumber)
{
    return false; // Room-jump cheats are disabled in every build.
}

bool AGhostReplayController::AdvanceToNextRoom(const ATargetPoint* NextRoomStart)
{
    if (bGameCompleted || !bInitialized || bRecording || bResetInProgress || !IsValid(PlayerRef) || !IsValid(ControllerRef))
    {
        return false;
    }
    StartTransform = PlayerRef->GetActorTransform();
    StartControlRotation = ControllerRef->GetControlRotation();
    if (CurrentRoomNumber != 10 && IsValid(NextRoomStart))
    {
        // The marker is floor-level; character actor origins are capsule centers.
        StartTransform.SetLocation(NextRoomStart->GetActorLocation()
            + FVector(0.f, 0.f, PlayerRef->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
        StartTransform.SetRotation(FRotator(0.f, NextRoomStart->GetActorRotation().Yaw, 0.f).Quaternion());
        StartControlRotation = FRotator(0.f, NextRoomStart->GetActorRotation().Yaw, 0.f);
    }
    WorkingSamples.Reset();
    WorkingButtonPresses.Reset();
    ClearSavedGhosts();
    Clock = 0.f;
    NextSampleTime = 0.f;
    NextButtonEvent = 0;
    ++CurrentRoomNumber;
    if (CurrentRoomNumber == 11)
    {
        bGameCompleted = true;
        RecordingWispBlend = 0.f;
        ControllerRef->SetIgnoreMoveInput(true);
        bCompletionInputLocked = true;
        PlayerRef->GetCharacterMovement()->StopMovementImmediately();
        PlayerRef->GetCharacterMovement()->DisableMovement();
        // Explicitly restart the completion clip, regardless of tutorial history.
        if (RoomNarration) { RoomNarration->ReplayRoom(11); }
        RemoveHUDRegistration();
        OnAttemptReset.Broadcast();
        Notify(TEXT("All ten rooms completed. Playing completion narration."));
        return true;
    }
    OnAttemptReset.Broadcast();
    Notify(FString::Printf(TEXT("Room %d ready. Echo cleared; R starts a new recording."), CurrentRoomNumber));
    return true;
}

bool AGhostReplayController::IsCompletionNarrationFinished() const
{
    return bGameCompleted && (!RoomNarration || RoomNarration->HasFinishedRoom(11));
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
    RemoveHUDRegistration();
    ReleaseResetInput();
    if (bCompletionInputLocked && IsValid(ControllerRef))
    {
        ControllerRef->SetIgnoreMoveInput(false);
        bCompletionInputLocked = false;
    }
    ClearSavedGhosts();
    if (GEngine)
    {
        GEngine->RemoveOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()));
    }
    Super::EndPlay(EndPlayReason);
}

float AGhostReplayController::WispOpacityAtUV(const FVector2D& UV)
{
    const float Edge = FMath::Min(FMath::Min(UV.X, 1.0 - UV.X), FMath::Min(UV.Y, 1.0 - UV.Y));
    // Feathered edge fog with warped fine filaments; the middle of the view is transparent.
    const float Curl = FMath::PerlinNoise2D(UV * 6.f + FVector2D(3.7, 8.1));
    const float Grain = FMath::PerlinNoise2D(UV * 19.f + FVector2D(17.2, 5.4));
    const float Width = .16f + .055f * Curl;
    const float Feather = FMath::Clamp(1.f - Edge / Width, 0.f, 1.f);
    const float Filament = FMath::Pow(.5f + .5f * FMath::Sin(Edge * 115.f + Curl * 9.f + UV.X * 14.f + UV.Y * 11.f), 3.f);
    return FMath::Clamp(Feather * Feather * (.15f + .55f * Filament + .15f * (Grain + 1.f)), 0.f, .85f);
}

void AGhostReplayController::CreateWispTexture()
{
    if (RecordingWispTexture) { return; }
    constexpr int32 Size = 512;
    RecordingWispTexture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
    if (!RecordingWispTexture || !RecordingWispTexture->GetPlatformData()) { return; }
    RecordingWispTexture->NeverStream = true;
    RecordingWispTexture->SRGB = true;
    RecordingWispTexture->Filter = TF_Bilinear;
    RecordingWispTexture->AddressX = TA_Clamp;
    RecordingWispTexture->AddressY = TA_Clamp;
    FColor* Pixels = static_cast<FColor*>(RecordingWispTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
    for (int32 Y = 0; Y < Size; ++Y)
    {
        for (int32 X = 0; X < Size; ++X)
        {
            const float Alpha = WispOpacityAtUV(FVector2D(X / float(Size - 1), Y / float(Size - 1)));
            Pixels[Y * Size + X] = FColor(225, 242, 255, FMath::RoundToInt(Alpha * 255.f));
        }
    }
    RecordingWispTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
    RecordingWispTexture->UpdateResource();
}

void AGhostReplayController::RemoveHUDRegistration()
{
    if (AHUD* HUD = RegisteredHUD.Get())
    {
        HUD->RemovePostRenderedActor(this);
        HUD->bShowOverlays = bPreviousShowOverlays;
    }
    RegisteredHUD.Reset();
}

void AGhostReplayController::UpdateHUDRegistration()
{
    const bool bNeedsHUD = bShowPhaseHUD || bShowControlsHUD;
    AHUD* HUD = bNeedsHUD && IsValid(ControllerRef) ? ControllerRef->GetHUD() : nullptr;
    if (bNeedsHUD && IsValid(ControllerRef) && !IsValid(HUD))
    {
        // Some template game modes explicitly leave HUD Class empty. Preserve any existing HUD.
        ControllerRef->SpawnDefaultHUD();
        HUD = ControllerRef->GetHUD();
    }
    if (RegisteredHUD.Get() == HUD) { return; }
    RemoveHUDRegistration();
    if (IsValid(HUD))
    {
        if (bShowPhaseHUD) { CreateWispTexture(); }
        RegisteredHUD = HUD;
        bPreviousShowOverlays = HUD->bShowOverlays;
        HUD->bShowOverlays = true;
        HUD->AddPostRenderedActor(this);
    }
}

void AGhostReplayController::PostRenderFor(APlayerController* PC, UCanvas* Canvas, FVector CameraPosition, FVector CameraDir)
{
    if (bGameCompleted) { return; }
    if (!IsValid(PC) || PC != ControllerRef || !Canvas) { return; }
    AHUD* HUD = RegisteredHUD.Get();
    if (!IsValid(HUD) || PC->GetHUD() != HUD || Canvas->SizeX <= 0 || Canvas->SizeY <= 0) { return; }
    if (!bShowPhaseHUD || RecordingWispBlend < .001f)
    {
        if (bShowControlsHUD) { DrawControlHints(HUD, Canvas); }
        return;
    }
    CreateWispTexture();
    if (!RecordingWispTexture)
    {
        if (bShowControlsHUD) { DrawControlHints(HUD, Canvas); }
        return;
    }
    const float Strength = FMath::Clamp(RecordingWispStrength, 0.f, 1.f) * RecordingWispBlend;
    FLinearColor Tint(1.f, 1.f, 1.f, Strength * .65f);
    HUD->DrawTexture(RecordingWispTexture, 0.f, 0.f, Canvas->SizeX, Canvas->SizeY,
        0.f, 0.f, 1.f, 1.f, Tint);
    // A second reversed layer drifts and breathes over the first, without covering the centre.
    const float Breath = 1.035f + .012f * FMath::Sin(WispAnimationTime * .6f);
    const float Width = Canvas->SizeX * Breath;
    const float Height = Canvas->SizeY * Breath;
    const float DriftX = Canvas->SizeX * .012f * FMath::Sin(WispAnimationTime * .43f);
    const float DriftY = Canvas->SizeY * .015f * FMath::Cos(WispAnimationTime * .37f);
    Tint.A = Strength * .55f;
    HUD->DrawTexture(RecordingWispTexture, (Canvas->SizeX - Width) * .5f + DriftX,
        (Canvas->SizeY - Height) * .5f + DriftY, Width, Height,
        0.f, 0.f, 1.f, 1.f, Tint, BLEND_Translucent, 1.f, false, 180.f, FVector2D(.5f, .5f));
    if (bShowControlsHUD) { DrawControlHints(HUD, Canvas); }
}

TArray<FString> AGhostReplayController::GetControlHints() const
{
    TArray<FString> Hints;
    Hints.Add(TEXT("WASD  Move   |   Mouse  Look"));
    Hints.Add(TEXT("Space  Jump"));
    if (CurrentRoomNumber > 0)
    {
        Hints.Add(TEXT("E  Press button"));
        if (GetGhostLimit() > 0 || bRecording) { Hints.Add(TEXT("R  Record / save ghost")); }
        Hints.Add(TEXT("X  Restart / cancel"));
        Hints.Add(TEXT("Z  Remove newest ghost"));
    }
    return Hints;
}

FString AGhostReplayController::GetGhostCounterText() const
{
    return FString::Printf(TEXT("Ghosts: %d / %d"), GetSavedGhostCount(), GetGhostLimit());
}

void AGhostReplayController::DrawControlHints(AHUD* HUD, UCanvas* Canvas) const
{
    if (!HUD || !Canvas || !GEngine) { return; }
    const TArray<FString> Hints = GetControlHints();
    const float Scale = FMath::Clamp(FMath::Min(Canvas->SizeX / 1280.f, Canvas->SizeY / 720.f), .4f, 1.5f);
    const float Padding = 12.f * Scale;
    const float Margin = 18.f * Scale;
    const float RowHeight = 22.f * Scale;
    float Width = 0.f;
    for (const FString& Hint : Hints)
    {
        float TextWidth, TextHeight;
        HUD->GetTextSize(Hint, TextWidth, TextHeight, GEngine->GetSmallFont(), Scale);
        Width = FMath::Max(Width, TextWidth);
    }
    const FString Counter = GetGhostCounterText();
    float CounterWidth, CounterHeight;
    HUD->GetTextSize(Counter, CounterWidth, CounterHeight, GEngine->GetSmallFont(), Scale * 1.15f);
    Width = FMath::Max(Width, CounterWidth);
    Width = FMath::Min(Width + 2.f * Padding, Canvas->SizeX - 2.f * Margin);
    const float Height = (Hints.Num() + 2) * RowHeight + 2.f * Padding;
    const float X = Canvas->SizeX - Margin - Width;
    const float Y = Canvas->SizeY - Margin - Height;
    HUD->DrawRect(FLinearColor(.015f, .025f, .035f, .65f), X, Y, Width, Height);
    const FLinearColor CounterColor = GetGhostLimit() > 0 && GetSavedGhostCount() >= GetGhostLimit()
        ? FLinearColor(1.f, .8f, .35f) : FLinearColor(.8f, .94f, 1.f);
    HUD->DrawText(Counter, CounterColor, X + Padding, Y + Padding, GEngine->GetSmallFont(), Scale * 1.15f);
    HUD->DrawText(TEXT("CONTROLS"), FLinearColor(.7f, .88f, 1.f), X + Padding, Y + Padding + RowHeight,
        GEngine->GetSmallFont(), Scale);
    for (int32 Index = 0; Index < Hints.Num(); ++Index)
    {
        HUD->DrawText(Hints[Index], FLinearColor(.9f, .93f, .96f), X + Padding,
            Y + Padding + (Index + 2) * RowHeight, GEngine->GetSmallFont(), Scale);
    }
}

