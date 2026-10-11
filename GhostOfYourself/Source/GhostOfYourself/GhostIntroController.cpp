#include "GhostIntroController.h"
#include "GhostGate.h"
#include "GhostReplayController.h"
#include "GhostRoomNarration.h"
#include "Engine/TargetPoint.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Sound/SoundBase.h"
#include "InputCoreTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogGhostIntro, Log, All);

AGhostIntroController::AGhostIntroController()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Narration = CreateDefaultSubobject<UAudioComponent>(TEXT("Narration"));
    Narration->SetupAttachment(RootComponent);
    Narration->SetAutoActivate(false);
    Narration->bAllowSpatialization = false;
    Narration->bSuppressSubtitles = false;
    ExitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ExitBox"));
    ExitBox->SetupAttachment(RootComponent);
    ExitBox->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
    ExitBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));
    ExitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ExitBox->SetGenerateOverlapEvents(false);
    ExitBox->SetHiddenInGame(true);
}

void AGhostIntroController::BeginPlay()
{
    Super::BeginPlay();
    Narration->OnAudioFinishedNative.AddUObject(this, &AGhostIntroController::OnNarrationFinished);
    if (IsValid(GateRef)) { GateRef->SetOpen(false); }
    else { UE_LOG(LogGhostIntro, Warning, TEXT("Assign the intro's Gate Ref before playing; level travel is disabled without it.")); }
}

bool AGhostIntroController::BeginIntroduction()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (bIntroStarted || !IsValid(PC) || !PC->IsLocalController() || !IsValid(PC->GetPawn())) { return false; }
    bIntroStarted = true;
    if (bUseRoomNarration && IsValid(ReplayControllerRef) && ReplayControllerRef->RoomNarration)
    {
        return true; // Room 0's complete playlist controls the welcome gate.
    }
    if (!IntroductionAudio)
    {
        IntroductionAudio = LoadObject<USoundBase>(nullptr,
            TEXT("/Game/GhostOfYourself/Audio/Intro_RobotWelcome.Intro_RobotWelcome"), nullptr, LOAD_NoWarn);
    }
    if (!IsValid(IntroductionAudio))
    {
        UE_LOG(LogGhostIntro, Warning, TEXT("Intro audio is missing; opening the door so the player is not trapped."));
        FinishIntroduction();
        return true;
    }
    NarrationDuration = FMath::Clamp(IntroductionAudio->GetDuration(), .1f, 120.f);
    NarrationElapsed = 0.f;
    Narration->SetSound(IntroductionAudio);
    Narration->Play();
    return true;
}

void AGhostIntroController::OnNarrationFinished(UAudioComponent* FinishedComponent)
{
    if (FinishedComponent == Narration && bIntroStarted && !bIntroFinished) { FinishIntroduction(); }
}

void AGhostIntroController::FinishIntroduction()
{
    if (bIntroFinished || bTravelling) { return; }
    bIntroStarted = true;
    bIntroFinished = true; // Set first: Stop can notify OnAudioFinished synchronously.
    if (bUseRoomNarration && IsValid(ReplayControllerRef) && ReplayControllerRef->RoomNarration)
    { ReplayControllerRef->RoomNarration->SkipRoom(0); }
    Narration->Stop();
    if (IsValid(GateRef)) { GateRef->SetOpen(true); }
}

bool AGhostIntroController::CanEnterAssessment(const AActor* ActorToCheck) const
{
    const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
    const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!bIntroFinished || bTravelling || bRoomZeroCompleted || !IsValid(ActorToCheck) || ActorToCheck != Player
        || !IsValid(PC) || !PC->IsLocalController() || !IsValid(GateRef)
        || !GateRef->bIsOpen || GateRef->bLockedClosed) { return false; }
    if (IsValid(ReplayControllerRef) && (ReplayControllerRef->CurrentRoomNumber != 0
        || ReplayControllerRef->bRecording || ReplayControllerRef->bResetInProgress)) { return false; }
    const FVector Point = ExitBox->GetComponentTransform().InverseTransformPosition(Player->GetActorLocation());
    const FVector Extent = ExitBox->GetUnscaledBoxExtent();
    return FMath::Abs(Point.X) <= Extent.X && FMath::Abs(Point.Y) <= Extent.Y && FMath::Abs(Point.Z) <= Extent.Z
        && GateRef->IsPlayerBeyondGate(Player, ExitBox->GetComponentLocation());
}

bool AGhostIntroController::TryEnterAssessment(const AActor* ActorToCheck)
{
    if (!CanEnterAssessment(ActorToCheck)) { return false; }
    if (IsValid(ReplayControllerRef))
    {
        if (!IsValid(RoomOneStart) || !GateRef->IsPositionBeyondGate(RoomOneStart->GetActorLocation(),
            ExitBox->GetComponentLocation(), Cast<ACharacter>(ActorToCheck)->GetCapsuleComponent()->GetScaledCapsuleRadius())) { return false; }
        if (!ReplayControllerRef->AdvanceToNextRoom(RoomOneStart)) { return false; }
        bRoomZeroCompleted = true;
        GateRef->LockClosed();
        return true;
    }
    if (!FPackageName::IsValidLongPackageName(AssessmentLevel.ToString())
        || !FPackageName::DoesPackageExist(AssessmentLevel.ToString()))
    {
        if (!bReportedMissingLevel)
        {
            UE_LOG(LogGhostIntro, Error, TEXT("Intro destination level does not exist: %s. Check Assessment Level and packaged map settings."), *AssessmentLevel.ToString());
            bReportedMissingLevel = true;
        }
        return false;
    }
    bTravelling = true;
    FadeElapsed = 0.f;
    TravelController = UGameplayStatics::GetPlayerController(this, 0);
    TravelController->SetIgnoreMoveInput(true);
    TravelController->SetIgnoreLookInput(true);
    bTravelInputLocked = true;
    if (TravelController->PlayerCameraManager)
    {
        TravelController->PlayerCameraManager->StartCameraFade(0.f, 1.f, FMath::Max(0.f, ExitFadeSeconds), FLinearColor::Black, false, true);
    }
    return true;
}

void AGhostIntroController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Step = FMath::Max(0.f, DeltaSeconds);
    if (IsValid(GateRef)) { GateRef->AdvanceMovement(Step); }
    if (bRoomZeroCompleted || (IsValid(ReplayControllerRef)
        && (ReplayControllerRef->CurrentRoomNumber != 0 || ReplayControllerRef->bResetInProgress))) { return; }
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (bTravelling)
    {
        if (!IsValid(PC) || !IsValid(Player))
        {
            ReleaseTravelInput();
            bTravelling = false;
            return;
        }
        Player->GetCharacterMovement()->StopMovementImmediately();
        FadeElapsed += Step;
        if (FadeElapsed >= FMath::Max(0.f, ExitFadeSeconds))
        {
            SetActorTickEnabled(false); // Submit only one travel request.
            UGameplayStatics::OpenLevel(this, AssessmentLevel);
        }
        return;
    }
    if (!IsValid(PC) || !PC->IsLocalController() || !IsValid(Player)) { return; }
    // Ignore the Enter key carried over from selecting BEGIN on the title screen.
    if (bAllowEnterToSkip && bIntroStarted && PC->WasInputKeyJustPressed(EKeys::Enter)) { FinishIntroduction(); }
    if (!bIntroStarted)
    {
        DelayElapsed += Step;
        if (DelayElapsed >= FMath::Max(0.f, StartDelaySeconds)) { BeginIntroduction(); }
    }
    else if (!bIntroFinished)
    {
        if (bUseRoomNarration && IsValid(ReplayControllerRef) && ReplayControllerRef->RoomNarration)
        {
            if (ReplayControllerRef->RoomNarration->HasFinishedRoom(0)) { FinishIntroduction(); }
        }
        else
        {
            NarrationElapsed += Step;
            if (NarrationElapsed >= NarrationDuration + .5f) { FinishIntroduction(); }
        }
    }
    TryEnterAssessment(Player);
}

void AGhostIntroController::ResetForRoomCheat(int32 RoomNumber)
{
    bIntroStarted = false;
    bIntroFinished = false;
    bTravelling = false;
    bRoomZeroCompleted = RoomNumber != 0;
    Narration->Stop();
    ReleaseTravelInput();
    DelayElapsed = NarrationElapsed = FadeElapsed = 0.f;
    SetActorTickEnabled(true);
    if (IsValid(GateRef))
    {
        if (RoomNumber == 0) { GateRef->ResetForRoomCheat(); }
        else { GateRef->LockClosed(); }
    }
}

void AGhostIntroController::ReleaseTravelInput()
{
    if (IsValid(TravelController))
    {
        if (bTravelInputLocked)
        {
            TravelController->SetIgnoreMoveInput(false);
            TravelController->SetIgnoreLookInput(false);
        }
        if (TravelController->PlayerCameraManager) { TravelController->PlayerCameraManager->StopCameraFade(); }
    }
    bTravelInputLocked = false;
}

void AGhostIntroController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Narration->OnAudioFinishedNative.RemoveAll(this);
    Narration->Stop();
    ReleaseTravelInput();
    Super::EndPlay(EndPlayReason);
}
