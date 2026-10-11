#include "GhostRoomNarration.h"
#include "GhostUserSettings.h"
#include "GhostReplayController.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

UGhostRoomNarration::UGhostRoomNarration()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
    SetAutoActivate(false);
    bAutoDestroy = false;
    bAllowSpatialization = false;
    // Room 11 is an internal completion state entered only after Room 10's exit.
    const int32 Rooms[] = { 0, 0, 0, 1, 2, 3, 6, 11 };
    const TCHAR* Names[] = { TEXT("GOY_IntroRoom"), TEXT("GOY_RecordInfo"), TEXT("GOY_CancelInfo"),
        TEXT("GOY_PressurePlate"), TEXT("GOY_Buttons"), TEXT("GOY_Lasers"), TEXT("GOY_MultipleGhosts"), TEXT("GOY_Completion") };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Rooms); ++Index)
    {
        const FString Path = FString::Printf(TEXT("/Game/GhostOfYourself/Audio/%s.%s"), Names[Index], Names[Index]);
        ConstructorHelpers::FObjectFinder<USoundBase> Clip(*Path);
        if (Clip.Succeeded()) { RoomPlaylists.FindOrAdd(Rooms[Index]).Clips.Add(Clip.Object); }
    }
}

void UGhostRoomNarration::BeginPlay()
{
    Super::BeginPlay();
    OnAudioFinishedNative.AddUObject(this, &UGhostRoomNarration::AdvanceClip);
    if (GetOwner()) { AddTickPrerequisiteActor(GetOwner()); }
    SetComponentTickEnabled(true);
}

void UGhostRoomNarration::StopSafely()
{
    bStopping = true;
    Stop();
    bStopping = false;
    bClipPlaying = false;
}

bool UGhostRoomNarration::IsNarrationPending() const
{
    const FGhostRoomVoicePlaylist* Playlist = RoomPlaylists.Find(NarrationRoom);
    if (bSequenceFinished || !Playlist) { return false; }
    for (const TObjectPtr<USoundBase>& Clip : Playlist->Clips) { if (IsValid(Clip)) { return true; } }
    return false;
}

void UGhostRoomNarration::ReplayRoom(int32 Room)
{
    StopSafely();
    NarrationRoom = Room;
    ClipIndex = 0;
    bSequenceFinished = false;
    WaitRemaining = FMath::Max(0.f, RoomStartDelay);
    ClipElapsed = 0.f;
    HeardRooms.Remove(Room);
    SetComponentTickEnabled(true);
}

void UGhostRoomNarration::SkipRoom(int32 Room)
{
    if (NarrationRoom != Room)
    {
        const AGhostReplayController* Replay = Cast<AGhostReplayController>(GetOwner());
        if (!Replay || Replay->CurrentRoomNumber != Room) { return; }
        NarrationRoom = Room;
    }
    bSequenceFinished = true;
    HeardRooms.Add(Room);
    StopSafely();
}

void UGhostRoomNarration::StartNextClip()
{
    const FGhostRoomVoicePlaylist* Playlist = RoomPlaylists.Find(NarrationRoom);
    while (Playlist && ClipIndex < Playlist->Clips.Num() && !IsValid(Playlist->Clips[ClipIndex])) { ++ClipIndex; }
    if (!Playlist || ClipIndex >= Playlist->Clips.Num())
    {
        bSequenceFinished = true;
        return;
    }
    USoundBase* Clip = Playlist->Clips[ClipIndex];
    ClipElapsed = 0.f;
    ClipDuration = FMath::Clamp(Clip->GetDuration(), .1f, 300.f);
    HeardRooms.Add(NarrationRoom);
    SetSound(Clip);
    SetVolumeMultiplier(GetDefault<UGhostUserSettings>()->NarratorVolume);
    bClipPlaying = true;
    Play();
}

void UGhostRoomNarration::AdvanceClip(UAudioComponent* Finished)
{
    if (bStopping || Finished != this || !bClipPlaying || bSequenceFinished) { return; }
    StopSafely();
    ++ClipIndex;
    WaitRemaining = FMath::Max(0.f, ClipGap);
}

void UGhostRoomNarration::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(DeltaSeconds, TickType, ThisTick);
    AGhostReplayController* Replay = Cast<AGhostReplayController>(GetOwner());
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!Replay || !IsValid(PC) || !PC->IsLocalController() || !IsValid(PC->GetPawn())) { return; }
    if (Replay->CurrentRoomNumber != NarrationRoom)
    {
        StopSafely();
        NarrationRoom = Replay->CurrentRoomNumber;
        ClipIndex = 0;
        bSequenceFinished = HeardRooms.Contains(NarrationRoom);
        WaitRemaining = FMath::Max(0.f, RoomStartDelay);
    }
    if (bSequenceFinished || Replay->bResetInProgress) { return; }
    const float Step = FMath::Max(0.f, DeltaSeconds);
    if (bClipPlaying)
    {
        ClipElapsed += Step;
        // Missing/muted devices must not permanently hold Room 0's welcome gate shut.
        if (ClipElapsed >= ClipDuration + .5f) { AdvanceClip(this); }
    }
    else
    {
        WaitRemaining -= Step;
        if (WaitRemaining <= 0.f) { StartNextClip(); }
    }
}

void UGhostRoomNarration::EndPlay(const EEndPlayReason::Type Reason)
{
    OnAudioFinishedNative.RemoveAll(this);
    StopSafely();
    Super::EndPlay(Reason);
}
