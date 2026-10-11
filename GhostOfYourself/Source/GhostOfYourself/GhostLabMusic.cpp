#include "GhostLabMusic.h"
#include "GhostUserSettings.h"
#include "GhostReplayController.h"
#include "GhostRoomNarration.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UGhostLabMusic::UGhostLabMusic()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
    SetAutoActivate(false);
    bAutoDestroy = false;
    bAllowSpatialization = false;
    MusicTrack = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/GhostOfYourself/Audio/GOY_LabMusic.GOY_LabMusic")));
}

void UGhostLabMusic::BeginPlay()
{
    Super::BeginPlay();
    if (const AGhostReplayController* Replay = Cast<AGhostReplayController>(GetOwner()))
    {
        if (Replay->RoomNarration) { AddTickPrerequisiteComponent(Replay->RoomNarration); }
    }
    SetComponentTickEnabled(true);
}

float UGhostLabMusic::GetTargetMusicVolume() const
{
    if (!bMusicEnabled) { return 0.f; }
    const AGhostReplayController* Replay = Cast<AGhostReplayController>(GetOwner());
    const bool bNarrating = Replay && Replay->RoomNarration && Replay->RoomNarration->IsNarrationPending();
    return FMath::Clamp(bNarrating ? NarrationVolume : AmbientVolume, 0.f, 1.f)
        * GetDefault<UGhostUserSettings>()->MusicVolume;
}

void UGhostLabMusic::RefreshSettingsVolume()
{
    CurrentVolume = GetTargetMusicVolume();
    SetVolumeMultiplier(CurrentVolume);
}

void UGhostLabMusic::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(DeltaSeconds, TickType, ThisTick);
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!IsValid(PC) || !PC->IsLocalController() || !IsValid(PC->GetPawn())) { return; }
    if (!bAttemptedStart && bMusicEnabled)
    {
        bAttemptedStart = true;
        if (USoundBase* Track = MusicTrack.LoadSynchronous())
        {
            SetSound(Track);
            CurrentVolume = GetTargetMusicVolume();
            SetVolumeMultiplier(CurrentVolume);
            FadeIn(FMath::Max(0.f, StartFadeSeconds), 1.f);
        }
        else { UE_LOG(LogTemp, Warning, TEXT("Import GOY_LabMusic using the supplied setup script before testing music.")); }
    }
    CurrentVolume = FMath::FInterpTo(CurrentVolume, GetTargetMusicVolume(), FMath::Max(0.f, DeltaSeconds), 3.f);
    SetVolumeMultiplier(CurrentVolume);
}

void UGhostLabMusic::EndPlay(const EEndPlayReason::Type Reason)
{
    Stop();
    Super::EndPlay(Reason);
}
