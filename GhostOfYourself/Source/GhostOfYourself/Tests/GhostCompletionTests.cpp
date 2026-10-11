#include "../GhostExitZone.h"
#include "../GhostGate.h"
#include "../GhostReplayController.h"
#include "../GhostRoomNarration.h"
#include "../RecordedGhost.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundBase.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostCompletionTest, "GhostOfYourself.Audio.Completion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGhostCompletionTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Init = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false).CreateFXSystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None,
        nullptr, true, ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("World"), World)) { return false; }
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->InitializeActorsForPlay(FURL());
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AGhostReplayController* Replay = World->SpawnActor<AGhostReplayController>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    AGhostExitZone* Exit = World->SpawnActor<AGhostExitZone>(FVector(0.f, 300.f, 0.f), FRotator::ZeroRotator, Spawn);
    AGhostGate* Gate = World->SpawnActor<AGhostGate>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    APlayerController* PC = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    ACharacter* Player = World->SpawnActor<ACharacter>(FVector(0.f, -300.f, 90.f), FRotator::ZeroRotator, Spawn);
    ARecordedGhost* Ghost = World->SpawnActor<ARecordedGhost>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Replay"), Replay) || !TestNotNull(TEXT("Exit"), Exit)
        || !TestNotNull(TEXT("Gate"), Gate) || !TestNotNull(TEXT("PC"), PC)
        || !TestNotNull(TEXT("Player"), Player) || !TestNotNull(TEXT("Ghost"), Ghost)) { return false; }
    PC->SetAsLocalPlayerController();
    PC->Possess(Player);
    Replay->CurrentRoomNumber = 10;
    Replay->bShowPhaseHUD = Replay->bShowControlsHUD = false;
    Replay->DispatchBeginPlay();
    Replay->Tick(0.f);
    UGhostRoomNarration* Voice = Replay->RoomNarration;
    const FGhostRoomVoicePlaylist* Completion = Voice->RoomPlaylists.Find(11);
    if (!Completion || Completion->Clips.Num() != 1
        || !TestNotNull(TEXT("Imported completion clip"), Completion->Clips[0].Get())) { return false; }
    TestEqual(TEXT("Completion clip name"), Completion->Clips[0]->GetName(), FString(TEXT("GOY_Completion")));
    TestFalse(TEXT("Entering Room 10 does not complete it"), Replay->bGameCompleted);
    Replay->GhostRef = Ghost;
    Replay->SavedSamples.AddDefaulted();
    Exit->ControllerRef = Replay;
    Exit->GateRef = Gate;
    Exit->FromRoomNumber = 10;
    Gate->DispatchBeginPlay();
    Exit->DispatchBeginPlay();
    Gate->SetOpen(true);
    Gate->AdvanceMovement(10.f);
    TestFalse(TEXT("Player must cross final exit"), Exit->TryCompleteForActor(Player));
    TestFalse(TEXT("Ghost cannot finish the game"), Exit->TryCompleteForActor(Ghost));
    Player->SetActorLocation(FVector(0.f, 300.f, 90.f));
    Replay->bRecording = true;
    TestFalse(TEXT("Recording cannot finish the game"), Exit->TryCompleteForActor(Player));
    Replay->bRecording = false;
    Gate->SetOpen(false);
    TestTrue(TEXT("Final box completes even if gate state is closed"), Exit->TryCompleteForActor(Player));
    TestTrue(TEXT("Completion and exit latch"), Replay->bGameCompleted && Exit->bCompleted);
    TestTrue(TEXT("Final gate locks"), Gate->bLockedClosed && !Gate->bIsOpen);
    TestTrue(TEXT("All ghosts are cleared"), Replay->GetSavedGhostCount() == 0 && !IsValid(Ghost));
    TestTrue(TEXT("Movement is held during narration"), PC->IsMoveInputIgnored()
        && Player->GetCharacterMovement()->MovementMode == MOVE_None);
    TestFalse(TEXT("Title return waits for narration"), Replay->IsCompletionNarrationFinished());
    Replay->ToggleRecording();
    Replay->CancelOrRestart();
    TestFalse(TEXT("Recording and restart blocked after completion"), Replay->bRecording || Replay->bResetInProgress);
    TestFalse(TEXT("Completion cannot fire twice"), Exit->TryCompleteForActor(Player));
    TestFalse(TEXT("Cheats cannot interrupt completion"), Replay->JumpToRoom(0));
    Voice->RoomStartDelay = 0.f;
    Voice->ReplayRoom(11);
    Voice->TickComponent(0.f, LEVELTICK_All, nullptr);
    TestTrue(TEXT("Completion narration ducks music"), Voice->IsNarrationPending());
    Voice->OnAudioFinishedNative.Broadcast(Voice);
    Voice->TickComponent(0.f, LEVELTICK_All, nullptr);
    TestFalse(TEXT("Title return respects narration end gap"), Replay->IsCompletionNarrationFinished());
    Voice->TickComponent(Voice->ClipGap + .01f, LEVELTICK_All, nullptr);
    TestTrue(TEXT("Clip finishing enables title return"), Replay->IsCompletionNarrationFinished());
    TestEqual(TEXT("Title destination"), Replay->CompletionTitleLevel,
        FName(TEXT("/Game/GhostOfYourself/Maps/L_Title")));
    Replay->Tick(.1f); // Begin fade; do not travel out of the transient test world.
    Replay->Destroy();
    TestFalse(TEXT("Completion input lock is balanced on shutdown"), PC->IsMoveInputIgnored());
    return true;
}
#endif
