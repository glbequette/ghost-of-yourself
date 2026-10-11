#include "../GhostButton.h"
#include "../GhostButtonGroup.h"
#include "../GhostGate.h"
#include "../GhostReplayController.h"
#include "../RecordedGhost.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostButtonTest, "GhostOfYourself.Puzzles.Buttons",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGhostButtonTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Init = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false).CreateFXSystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None,
        nullptr, true, ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("Transient world"), World)) { return false; }
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->InitializeActorsForPlay(FURL());
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AGhostReplayController* Replay = World->SpawnActor<AGhostReplayController>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    APlayerController* PC = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    ACharacter* Player = World->SpawnActor<ACharacter>(FVector(0.f, 0.f, 90.f), FRotator::ZeroRotator, Spawn);
    AGhostGate* Gate = World->SpawnActor<AGhostGate>(FVector(2000.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
    AGhostButtonGroup* Group = World->SpawnActor<AGhostButtonGroup>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    AGhostButton* A = World->SpawnActor<AGhostButton>(FVector(0.f, 0.f, 120.f), FRotator::ZeroRotator, Spawn);
    AGhostButton* B = World->SpawnActor<AGhostButton>(FVector(1000.f, 0.f, 120.f), FRotator::ZeroRotator, Spawn);
    AGhostButton* Other = World->SpawnActor<AGhostButton>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Replay"), Replay) || !TestNotNull(TEXT("PC"), PC)
        || !TestNotNull(TEXT("Player"), Player) || !TestNotNull(TEXT("Gate"), Gate)
        || !TestNotNull(TEXT("Group"), Group) || !TestNotNull(TEXT("Button A"), A)
        || !TestNotNull(TEXT("Button B"), B) || !TestNotNull(TEXT("Other button"), Other)) { return false; }
    PC->SetAsLocalPlayerController();
    PC->Possess(Player);
    Replay->CurrentRoomNumber = 2;
    Replay->bUseResetFade = false;
    Replay->bShowDebugStatus = false;
    Replay->Tick(0.f);
    Group->bShowDebugStatus = false;
    Group->ControllerRef = Replay;
    Group->GateRef = Gate;
    A->GroupRef = Group;
    B->GroupRef = Group;
    Gate->DispatchBeginPlay();
    Group->DispatchBeginPlay();

    TestFalse(TEXT("No presses starts closed"), Gate->bIsOpen);
    TestFalse(TEXT("Null target rejected"), Group->ReceivePress(nullptr, 1.f, Player->GetActorLocation()));
    TestFalse(TEXT("Unlinked target rejected"), Group->ReceivePress(Other, 1.f, FVector::ZeroVector));
    TestFalse(TEXT("Remote press rejected"), Group->ReceivePress(A, 1.f, B->GetActorLocation()));
    TestTrue(TEXT("A accepts nearby press"), Group->ReceivePress(A, 1.f, A->GetActorLocation()));
    TestFalse(TEXT("Used button rejects second press"), Group->ReceivePress(A, 1.1f, A->GetActorLocation()));
    TestTrue(TEXT("Used state exposed on button"), A->bUsedThisAttempt);
    TestFalse(TEXT("Repeated A is not two distinct buttons"), Group->bUnlocked);
    Group->ReceivePress(B, 2.5f, B->GetActorLocation());
    TestTrue(TEXT("Inclusive 1.5 second boundary unlocks"), Group->bUnlocked && Gate->bIsOpen);
    Replay->Clock = 100.f;
    Group->Tick(.1f);
    TestTrue(TEXT("Success stays unlocked after timestamps expire"), Group->bUnlocked && Gate->bIsOpen);
    Group->ResetPuzzle();
    TestFalse(TEXT("Reset closes gate and clears latch"), Group->bUnlocked || Gate->bIsOpen);
    TestFalse(TEXT("Reset rearms button"), A->bUsedThisAttempt || Group->IsButtonUsed(A));
    Group->ReceivePress(A, 1.f, A->GetActorLocation());
    Group->ReceivePress(B, 2.6f, B->GetActorLocation());
    TestFalse(TEXT("Stale press cannot combine with new button"), Group->bUnlocked);
    TestFalse(TEXT("Expiry does not allow pressing same button again"), Group->ReceivePress(A, 2.7f, A->GetActorLocation()));
    Group->ResetPuzzle();
    Replay->CurrentRoomNumber = 1;
    TestFalse(TEXT("Future room is inactive"), Group->ReceivePress(A, 0.f, A->GetActorLocation()));
    Replay->CurrentRoomNumber = 2;
    Replay->bResetInProgress = true;
    TestFalse(TEXT("Fade blocks presses"), Group->ReceivePress(A, 0.f, A->GetActorLocation()));
    Replay->bResetInProgress = false;
    Other->GroupRef = Group;
    TestTrue(TEXT("Three-button group accepts presses"), Group->ReceivePress(A, 0.f, A->GetActorLocation()));
    TestFalse(TEXT("One press does not unlock three buttons"), Group->bUnlocked);
    Other->GroupRef = nullptr;
    Group->ResetPuzzle();

    Replay->ToggleRecording();
    TestTrue(TEXT("Recording starts"), Replay->bRecording);
    Replay->Clock = 8.f;
    TestTrue(TEXT("Live E path records press"), Replay->TryPressButton(A));
    TestFalse(TEXT("Repeated live press is not recorded"), Replay->TryPressButton(A));
    Player->SetActorLocation(FVector(500.f, 0.f, 90.f));
    Replay->Clock = 10.f;
    Replay->ToggleRecording();
    TestEqual(TEXT("Save captures one event"), Replay->SavedButtonPresses.Num(), 1);
    if (Replay->SavedButtonPresses.Num() != 1 || !TestNotNull(TEXT("Recorded ghost spawned"), Replay->GhostRef.Get())) { return false; }
    TestEqual(TEXT("Event has exact timestamp"), Replay->SavedButtonPresses[0].Time, 8.f);
    TestTrue(TEXT("Event remembers target"), Replay->SavedButtonPresses[0].Button.Get() == A);
    TestFalse(TEXT("Commit resets puzzle"), Group->bUnlocked || Gate->bIsOpen);
    Replay->Tick(9.f); // Cross event time in one frame; ghost is now beyond A's interaction range.
    Player->SetActorLocation(FVector(1000.f, 0.f, 90.f));
    TestTrue(TEXT("Player presses B"), Replay->TryPressButton(B));
    TestTrue(TEXT("Crossed event validates pose at its own timestamp and combines with live press"), Group->bUnlocked);
    Group->ResetPuzzle();
    Replay->Tick(0.f);
    Replay->TryPressButton(B);
    TestFalse(TEXT("Event is emitted once, not every frame"), Group->bUnlocked);
    Replay->CancelOrRestart();
    TestTrue(TEXT("Restart resets shared clock"), FMath::IsNearlyZero(Replay->Clock));
    Player->SetActorLocation(FVector(1000.f, 0.f, 90.f));
    Replay->Tick(8.1f);
    Replay->TryPressButton(B);
    TestTrue(TEXT("Restart emits saved event again"), Group->bUnlocked);
    Group->ResetPuzzle();
    Replay->Tick(100.f);
    Replay->TryPressButton(B);
    TestFalse(TEXT("Held endpoint does not keep pressing"), Group->bUnlocked);

    Replay->CancelOrRestart();
    Replay->SavedButtonPresses[0].Button = B; // Recorded pose at A cannot activate remote B.
    Replay->Tick(8.1f);
    Player->SetActorLocation(FVector(0.f, 0.f, 90.f));
    Replay->TryPressButton(A);
    TestFalse(TEXT("Replay checks target distance"), Group->bUnlocked);
    Replay->CancelOrRestart();
    Replay->SavedButtonPresses[0].Button = Other;
    Other->Destroy();
    Replay->Tick(8.1f);
    TestFalse(TEXT("Destroyed replay target is safe"), Group->bUnlocked);
    Replay->UndoGhost();
    TestTrue(TEXT("Undo clears events and ghost"), Replay->SavedButtonPresses.IsEmpty() && Replay->GhostRef == nullptr);
    Replay->ToggleRecording();
    Replay->Clock = 2.f;
    Replay->TryPressButton(A);
    Replay->CancelOrRestart();
    Replay->ToggleRecording();
    Replay->Clock = 3.f;
    Replay->ToggleRecording();
    TestTrue(TEXT("Canceled interactions do not leak into next recording"), Replay->SavedButtonPresses.IsEmpty());
    TestTrue(TEXT("Transition succeeds"), Replay->AdvanceToNextRoom(nullptr));
    TestTrue(TEXT("Room transition clears samples and events"), Replay->SavedSamples.IsEmpty() && Replay->SavedButtonPresses.IsEmpty());
    TestFalse(TEXT("Room transition resets puzzle"), Group->bUnlocked);
    Gate->LockClosed();
    Group->ResetPuzzle();
    TestTrue(TEXT("Reset preserves completed room's locked gate"), Gate->bLockedClosed && !Gate->bIsOpen);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostButtonCountTest, "GhostOfYourself.Puzzles.ButtonsAnyCount",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGhostButtonCountTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Init = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false).CreateFXSystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None,
        nullptr, true, ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("Transient world"), World)) { return false; }
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->InitializeActorsForPlay(FURL());
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AGhostReplayController* Replay = World->SpawnActor<AGhostReplayController>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    AGhostGate* Gate = World->SpawnActor<AGhostGate>(FVector(10000.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
    AGhostButtonGroup* Group = World->SpawnActor<AGhostButtonGroup>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Replay"), Replay) || !TestNotNull(TEXT("Gate"), Gate) || !TestNotNull(TEXT("Group"), Group)) { return false; }
    Replay->CurrentRoomNumber = 2;
    Group->ControllerRef = Replay;
    Group->GateRef = Gate;
    Group->bShowDebugStatus = false;
    TArray<AGhostButton*> Buttons;
    for (int32 Index = 0; Index < 12; ++Index)
    {
        AGhostButton* Button = World->SpawnActor<AGhostButton>(FVector(Index * 500.f, 0.f, 120.f), FRotator::ZeroRotator, Spawn);
        if (!TestNotNull(TEXT("Button"), Button)) { return false; }
        Button->GroupRef = Group;
        Buttons.Add(Button);
    }
    Gate->DispatchBeginPlay();
    Group->DispatchBeginPlay();
    for (int32 Index = 0; Index < 11; ++Index)
    {
        TestTrue(TEXT("Large group accepts each press"), Group->ReceivePress(Buttons[Index], Index * .1f, Buttons[Index]->GetActorLocation()));
        TestFalse(TEXT("Gate waits for every member"), Group->bUnlocked);
    }
    TestFalse(TEXT("Large group rejects used button"), Group->ReceivePress(Buttons[0], 1.05f, Buttons[0]->GetActorLocation()));
    TestFalse(TEXT("Repeated button cannot replace missing member"), Group->bUnlocked);
    Group->ReceivePress(Buttons[11], 1.1f, Buttons[11]->GetActorLocation());
    TestTrue(TEXT("All twelve buttons unlock"), Group->bUnlocked && Gate->bIsOpen);
    Group->ResetPuzzle();
    for (int32 Index = 0; Index < 12; ++Index)
    {
        Group->ReceivePress(Buttons[Index], Index * .3f, Buttons[Index]->GetActorLocation());
    }
    TestFalse(TEXT("Large group still enforces sync window"), Group->bUnlocked);
    for (int32 Index = 1; Index < 12; ++Index) { Buttons[Index]->GroupRef = nullptr; }
    Group->ResetPuzzle();
    TestTrue(TEXT("Single-button group accepts press"), Group->ReceivePress(Buttons[0], 0.f, Buttons[0]->GetActorLocation()));
    TestTrue(TEXT("Single-button group unlocks"), Group->bUnlocked);
    Buttons[0]->GroupRef = nullptr;
    Group->ResetPuzzle();
    TestFalse(TEXT("Empty group stays closed"), Group->bUnlocked || Gate->bIsOpen);
    return true;
}
#endif
