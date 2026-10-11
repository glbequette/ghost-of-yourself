#include "../GhostRoomNarration.h"
#include "../GhostReplayController.h"
#include "../GhostIntroController.h"
#include "../GhostGate.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundBase.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostRoomNarrationTest, "GhostOfYourself.Audio.RoomNarration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGhostRoomNarrationTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Init = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false).CreateFXSystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None,
        nullptr, true, ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("World"), World)) { return false; }
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    World->InitializeActorsForPlay(FURL());
    AGhostReplayController* Replay = World->SpawnActor<AGhostReplayController>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    APlayerController* PC = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    ACharacter* Player = World->SpawnActor<ACharacter>(FVector(-500.f, 0.f, 90.f), FRotator::ZeroRotator, Spawn);
    AGhostIntroController* Intro = World->SpawnActor<AGhostIntroController>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    AGhostGate* Gate = World->SpawnActor<AGhostGate>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Replay"), Replay) || !TestNotNull(TEXT("PC"), PC)
        || !TestNotNull(TEXT("Player"), Player) || !TestNotNull(TEXT("Intro"), Intro)
        || !TestNotNull(TEXT("Gate"), Gate)) { return false; }
    PC->SetAsLocalPlayerController();
    PC->Possess(Player);
    Replay->CurrentRoomNumber = 0;
    Replay->bShowControlsHUD = Replay->bShowPhaseHUD = false;
    Replay->DispatchBeginPlay();
    UGhostRoomNarration* Voice = Replay->RoomNarration;
    if (!TestNotNull(TEXT("Automatic narration component"), Voice)) { return false; }
    const FGhostRoomVoicePlaylist* Zero = Voice->RoomPlaylists.Find(0);
    if (!Zero || Zero->Clips.Num() != 3) { AddError(TEXT("Room 0 must have three imported clips")); return false; }
    const TCHAR* Expected[] = { TEXT("GOY_IntroRoom"), TEXT("GOY_RecordInfo"), TEXT("GOY_CancelInfo") };
    for (int32 Index = 0; Index < 3; ++Index)
    {
        if (!TestNotNull(TEXT("Imported clip"), Zero->Clips[Index].Get())) { return false; }
        TestEqual(TEXT("Room 0 playlist order"), Zero->Clips[Index]->GetName(), FString(Expected[Index]));
    }
    const int32 Rooms[] = {1, 2, 3, 6};
    const TCHAR* Names[] = {TEXT("GOY_PressurePlate"), TEXT("GOY_Buttons"), TEXT("GOY_Lasers"), TEXT("GOY_MultipleGhosts")};
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FGhostRoomVoicePlaylist* List = Voice->RoomPlaylists.Find(Rooms[Index]);
        if (!List || List->Clips.Num() != 1 || !IsValid(List->Clips[0])) { AddError(TEXT("Tutorial clip missing")); return false; }
        TestEqual(TEXT("Room clip mapping"), List->Clips[0]->GetName(), FString(Names[Index]));
    }
    Voice->RoomStartDelay = Voice->ClipGap = 0.f;
    Intro->ReplayControllerRef = Replay;
    Intro->GateRef = Gate;
    Gate->DispatchBeginPlay();
    Intro->DispatchBeginPlay();
    TestTrue(TEXT("Intro delegates to room sequence"), Intro->BeginIntroduction());
    Voice->TickComponent(0.f, LEVELTICK_All, nullptr);
    TestFalse(TEXT("Welcome gate stays closed during first clip"), Gate->bIsOpen);
    for (int32 Index = 0; Index < 3; ++Index)
    {
        TestEqual(TEXT("Playback follows ordered clips"), Voice->ClipIndex, Index);
        Voice->OnAudioFinishedNative.Broadcast(Voice);
        Voice->TickComponent(0.f, LEVELTICK_All, nullptr);
        Intro->Tick(0.f);
        if (Index < 2) { TestFalse(TEXT("Gate waits for remaining clips"), Gate->bIsOpen); }
    }
    TestTrue(TEXT("Room 0 completion opens gate"), Voice->HasFinishedRoom(0) && Gate->bIsOpen);
    Replay->CurrentRoomNumber = 1;
    Voice->TickComponent(0.f, LEVELTICK_All, nullptr);
    TestEqual(TEXT("Room entry selects next clip"), Voice->NarrationRoom, 1);
    Voice->OnAudioFinishedNative.Broadcast(Voice);
    Voice->TickComponent(0.f, LEVELTICK_All, nullptr);
    Replay->OnAttemptReset.Broadcast();
    Voice->TickComponent(0.f, LEVELTICK_All, nullptr);
    TestTrue(TEXT("Attempt reset does not repeat voice"), Voice->HasFinishedRoom(1));
    Voice->ReplayRoom(1);
    Voice->TickComponent(0.f, LEVELTICK_All, nullptr);
    TestFalse(TEXT("Developer revisit can replay tutorial"), Voice->HasFinishedRoom(1));
    Voice->SkipRoom(1);
    TestTrue(TEXT("Skip stops entire sequence"), Voice->HasFinishedRoom(1));
    return true;
}
#endif
