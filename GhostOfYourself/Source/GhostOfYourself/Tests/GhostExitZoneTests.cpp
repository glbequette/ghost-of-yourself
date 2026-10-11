#include "../GhostExitZone.h"
#include "../GhostGate.h"
#include "../GhostReplayController.h"
#include "../RecordedGhost.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostExitZoneTest, "GhostOfYourself.Puzzles.ExitZone",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGhostExitZoneTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Init = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false).CreateFXSystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None,
        nullptr, true, ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("Transient world exists"), World)) { return false; }
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->InitializeActorsForPlay(FURL());

    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AGhostExitZone* Exit = World->SpawnActor<AGhostExitZone>(FVector(0.f, 300.f, 0.f), FRotator::ZeroRotator, Spawn);
    AGhostGate* Gate = World->SpawnActor<AGhostGate>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    AGhostReplayController* Replay = World->SpawnActor<AGhostReplayController>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    APlayerController* PC = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    ACharacter* Player = World->SpawnActor<ACharacter>(FVector(0.f, -300.f, 90.f), FRotator::ZeroRotator, Spawn);
    ACharacter* Other = World->SpawnActor<ACharacter>(FVector(0.f, 300.f, 90.f), FRotator::ZeroRotator, Spawn);
    ARecordedGhost* Ghost = World->SpawnActor<ARecordedGhost>(FVector(0.f, 300.f, 90.f), FRotator::ZeroRotator, Spawn);
    ATargetPoint* Start = World->SpawnActor<ATargetPoint>(FVector(0.f, 500.f, 0.f), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Exit"), Exit) || !TestNotNull(TEXT("Gate"), Gate)
        || !TestNotNull(TEXT("Replay"), Replay) || !TestNotNull(TEXT("Controller"), PC)
        || !TestNotNull(TEXT("Player"), Player) || !TestNotNull(TEXT("Other character"), Other)
        || !TestNotNull(TEXT("Ghost"), Ghost) || !TestNotNull(TEXT("Next start"), Start)) { return false; }
    PC->SetAsLocalPlayerController();
    PC->Possess(Player);
    if (!TestTrue(TEXT("Test controller is local"), PC->IsLocalController())
        || !TestTrue(TEXT("Test player is possessed and discoverable"), PC->GetPawn() == Player
            && UGameplayStatics::GetPlayerCharacter(Exit, 0) == Player)) { return false; }
    Replay->bShowDebugStatus = false;
    Replay->bUseResetFade = false; // Test room rules independently of presentation timing.
    Replay->Tick(0.f);
    Replay->GhostRef = Ghost;
    FRecordedGhostSample Sample;
    Sample.Transform = Ghost->GetActorTransform();
    Replay->SavedSamples.Add(Sample);
    Replay->Clock = 10.f;
    Exit->ControllerRef = Replay;
    Exit->GateRef = Gate;
    Exit->NextRoomStart = Start;
    Gate->DispatchBeginPlay();
    Exit->DispatchBeginPlay();
    TestFalse(TEXT("Floating exit text is hidden"), Exit->ExitLabel->IsVisible());
    Gate->SetOpen(true);
    Gate->AdvanceMovement(10.f);
    TestFalse(TEXT("Null actor cannot transition"), Exit->TryCompleteForActor(nullptr));
    TestFalse(TEXT("Player outside zone cannot transition"), Exit->TryCompleteForActor(Player));
    TestFalse(TEXT("Ghost cannot transition"), Exit->TryCompleteForActor(Ghost));
    TestFalse(TEXT("Unpossessed character cannot transition"), Exit->TryCompleteForActor(Other));
    Player->SetActorLocation(FVector(0.f, 300.f, 90.f));
    Replay->bRecording = true;
    TestFalse(TEXT("Recording blocks transition"), Exit->TryCompleteForActor(Player));
    Replay->bRecording = false;
    Gate->SetOpen(false);
    Gate->AdvanceMovement(10.f); // Fully close the physical door, not just its requested state.
    TestFalse(TEXT("Closed gate blocks transition"), Exit->TryCompleteForActor(Player));
    Gate->SetOpen(true);
    Exit->FromRoomNumber = 2;
    TestFalse(TEXT("Future room exit cannot transition early"), Exit->TryCompleteForActor(Player));
    Exit->FromRoomNumber = 1;
    Exit->GateRef = nullptr;
    TestFalse(TEXT("Missing gate disables transition"), Exit->TryCompleteForActor(Player));
    Exit->GateRef = Gate;
    Exit->ControllerRef = nullptr;
    TestFalse(TEXT("Missing controller disables transition"), Exit->TryCompleteForActor(Player));
    Exit->ControllerRef = Replay;
    Start->SetActorLocation(FVector(0.f, -100.f, 0.f));
    TestFalse(TEXT("Checkpoint behind gate is rejected"), Exit->TryCompleteForActor(Player));
    Start->SetActorLocation(FVector(0.f, 500.f, 0.f));

    Exit->ExitBox->SetBoxExtent(FVector(100.f, 1000.f, 100.f));
    Player->SetActorLocation(FVector(0.f, -20.f, 90.f));
    TestFalse(TEXT("An overlapping exit box cannot trigger on room-one side"), Exit->TryCompleteForActor(Player));
    Player->SetActorLocation(FVector(0.f, 20.f, 90.f));
    TestFalse(TEXT("Capsule must fully clear doorway before locking"), Exit->TryCompleteForActor(Player));
    Exit->ExitBox->SetBoxExtent(FVector(100.f));
    Exit->SetActorTransform(FTransform(FRotator(0.f, 40.f, 0.f), FVector(0.f, 300.f, 0.f), FVector(2.f, 0.5f, 1.f)));
    const FTransform Box = Exit->ExitBox->GetComponentTransform();
    Player->SetActorLocation(Box.TransformPosition(FVector(101.f, 0.f, 0.f)));
    TestFalse(TEXT("Scaled box rejects outside position"), Exit->TryCompleteForActor(Player));
    Player->SetActorLocation(Box.TransformPosition(FVector(99.f, 99.f, 0.f)));
    const FVector EntryPosition = Player->GetActorLocation();
    TestTrue(TEXT("Player past door in rotated/scaled zone transitions"), Exit->TryCompleteForActor(Player));
    TestTrue(TEXT("Transition is latched"), Exit->bCompleted);
    TestEqual(TEXT("Room advances once"), Replay->CurrentRoomNumber, 2);
    TestFalse(TEXT("Saved ghost was destroyed"), IsValid(Ghost));
    TestTrue(TEXT("Ghost reference and slot are cleared"), Replay->GhostRef == nullptr && Replay->SavedSamples.IsEmpty());
    TestTrue(TEXT("Room clock clears"), FMath::IsNearlyZero(Replay->Clock));
    TestTrue(TEXT("Player keeps walking without teleporting on exit"), Player->GetActorLocation().Equals(EntryPosition));
    TestTrue(TEXT("Gate is locked closed"), Gate->bLockedClosed && !Gate->bIsOpen);
    TestTrue(TEXT("Immediate return barrier blocks before door finishes lowering"),
        Gate->LockBarrier->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics);
    Gate->SetOpen(true);
    TestFalse(TEXT("Old pressure plate cannot reopen a locked gate"), Gate->bIsOpen);
    Gate->AdvanceMovement(10.f);
    TestTrue(TEXT("Visible door lowers to original position"), Gate->GateMesh->GetRelativeLocation().Equals(FVector(0.f, 0.f, 125.f), 0.01f));
    TestFalse(TEXT("Completed exit cannot advance again"), Exit->TryCompleteForActor(Player));

    const FVector RoomTwoStart = Start->GetActorLocation() + FVector(0.f, 0.f, Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    Replay->ToggleRecording();
    TestTrue(TEXT("Empty room-two slot can record"), Replay->bRecording);
    TestTrue(TEXT("R resets to room-two checkpoint"), Player->GetActorLocation().Equals(RoomTwoStart, 0.01f));
    TestTrue(TEXT("Completed exit stays consumed during room-two recording"), Exit->bCompleted);
    Replay->CancelOrRestart();
    Replay->UndoGhost();
    TestEqual(TEXT("X/Z do not return to room one"), Replay->CurrentRoomNumber, 2);
    TestTrue(TEXT("X/Z reset to room-two checkpoint"), Player->GetActorLocation().Equals(RoomTwoStart, 0.01f));
    TestTrue(TEXT("Room-one gate stays locked after reset"), Gate->bLockedClosed);

    AGhostExitZone* ExitTwo = World->SpawnActor<AGhostExitZone>(FVector(0.f, 900.f, 0.f), FRotator::ZeroRotator, Spawn);
    AGhostGate* GateTwo = World->SpawnActor<AGhostGate>(FVector(0.f, 700.f, 0.f), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Second exit"), ExitTwo) || !TestNotNull(TEXT("Second gate"), GateTwo)) { return false; }
    ExitTwo->ControllerRef = Replay;
    ExitTwo->GateRef = GateTwo;
    ExitTwo->FromRoomNumber = 2;
    GateTwo->SetOpen(true);
    Player->SetActorLocation(FVector(0.f, 900.f, 90.f));
    TestTrue(TEXT("Next exit without marker captures current position"), ExitTwo->TryCompleteForActor(Player));
    Replay->CancelOrRestart();
    TestTrue(TEXT("Captured checkpoint survives restart"), Player->GetActorLocation().Equals(FVector(0.f, 900.f, 90.f), 0.01f));
    TestEqual(TEXT("Second transition advances exactly once"), Replay->CurrentRoomNumber, 3);

    // Exercise the real fade state machine without rendering or real-time sleeps.
    if (!TestNotNull(TEXT("Player camera manager exists for reset fade"), PC->PlayerCameraManager.Get())) { return false; }
    Replay->bUseResetFade = true;
    Replay->FadeOutSeconds = 0.1f;
    Replay->FadeInSeconds = 0.15f;
    Player->SetActorLocation(FVector(0.f, 1100.f, 90.f));
    const FVector BeforeFade = Player->GetActorLocation();
    Replay->Clock = 3.f;
    PC->SetIgnoreMoveInput(true); // Another system's lock must survive our fade.
    Replay->ToggleRecording();
    TestTrue(TEXT("Recording reset starts a fade"), Replay->bResetInProgress);
    TestFalse(TEXT("Recording waits until fade-in finishes"), Replay->bRecording);
    TestTrue(TEXT("No immediate teleport before fade-out"), Player->GetActorLocation().Equals(BeforeFade));
    TestTrue(TEXT("Look input locks during fade"), PC->IsLookInputIgnored());
    TestFalse(TEXT("Room transition cannot happen during reset"), Replay->AdvanceToNextRoom(nullptr));
    Replay->Tick(0.04f);
    TestTrue(TEXT("Fade-out holds current position and pauses clock"),
        Player->GetActorLocation().Equals(BeforeFade) && FMath::IsNearlyEqual(Replay->Clock, 3.f));
    Replay->Tick(0.07f);
    TestTrue(TEXT("At black, teleport reaches the current room checkpoint"),
        Player->GetActorLocation().Equals(FVector(0.f, 900.f, 90.f), 0.01f));
    TestTrue(TEXT("Clock resets while black"), FMath::IsNearlyZero(Replay->Clock));
    TestFalse(TEXT("Recording stays paused during fade-in"), Replay->bRecording);
    Replay->Tick(0.16f);
    TestTrue(TEXT("Recording starts after fade-in"), Replay->bRecording && !Replay->bResetInProgress);
    TestFalse(TEXT("Look input unlocks after fade"), PC->IsLookInputIgnored());
    TestTrue(TEXT("External movement lock survives our balanced unlock"), PC->IsMoveInputIgnored());
    PC->SetIgnoreMoveInput(false);
    TestFalse(TEXT("No stray movement lock remains"), PC->IsMoveInputIgnored());
    Replay->Tick(0.05f);
    Replay->ToggleRecording();
    TestTrue(TEXT("Save begins another reset fade"), Replay->bResetInProgress);
    TestTrue(TEXT("Fade time is excluded from saved timestamps"), !Replay->SavedSamples.IsEmpty()
        && FMath::IsNearlyEqual(Replay->SavedSamples.Last().Time, 0.05f, 0.001f));
    const int32 SavedCount = Replay->SavedSamples.Num();
    Replay->UndoGhost();
    TestEqual(TEXT("Repeated input during fade cannot erase the saved attempt"), Replay->SavedSamples.Num(), SavedCount);
    Replay->Tick(0.11f);
    Replay->Tick(0.16f);
    TestFalse(TEXT("Saving fade finishes"), Replay->bResetInProgress);
    TestTrue(TEXT("Playback starts at time zero after fade-in"), FMath::IsNearlyZero(Replay->Clock));
    Replay->bUseResetFade = false;
    Replay->UndoGhost();

    // Packaged frame hitches can jump completely over a thin exit box.
    AGhostGate* ThinGate = World->SpawnActor<AGhostGate>(FVector(0.f, 1200.f, 0.f), FRotator::ZeroRotator, Spawn);
    AGhostExitZone* ThinExit = World->SpawnActor<AGhostExitZone>(FVector(0.f, 1270.f, 0.f), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Thin gate"), ThinGate) || !TestNotNull(TEXT("Thin exit"), ThinExit)) { return false; }
    ThinExit->ControllerRef = Replay;
    ThinExit->GateRef = ThinGate;
    ThinExit->FromRoomNumber = 3;
    ThinExit->ExitBox->SetBoxExtent(FVector(100.f, 25.f, 100.f));
    ThinGate->DispatchBeginPlay();
    ThinExit->DispatchBeginPlay();
    Player->SetActorLocation(FVector(0.f, 1220.f, 90.f));
    ThinExit->Tick(0.f);
    Player->SetActorLocation(FVector(0.f, 1340.f, 90.f));
    ThinExit->Tick(.3f);
    TestFalse(TEXT("Sweeping over exit cannot bypass closed gate"), ThinExit->bCompleted);
    ThinGate->SetOpen(true);
    Player->SetActorLocation(FVector(0.f, 1220.f, 90.f));
    ThinExit->Tick(0.f);
    Replay->bRecording = true;
    Player->SetActorLocation(FVector(0.f, 1340.f, 90.f));
    ThinExit->Tick(.3f);
    Replay->bRecording = false;
    ThinExit->Tick(0.f);
    TestFalse(TEXT("Recording movement is not carried into a live exit sweep"), ThinExit->bCompleted);
    Player->SetActorLocation(FVector(0.f, 1220.f, 90.f));
    ThinExit->Tick(0.f);
    Replay->OnAttemptReset.Broadcast();
    Player->SetActorLocation(FVector(0.f, 1340.f, 90.f));
    ThinExit->Tick(0.f);
    TestFalse(TEXT("Reset teleports cannot activate a swept exit"), ThinExit->bCompleted);
    Player->SetActorLocation(FVector(0.f, 1220.f, 90.f));
    ThinExit->Tick(0.f);
    ThinGate->AdvanceMovement(2.f);
    ThinGate->SetOpen(false);
    TestFalse(TEXT("Closing door has lost logical open permission"), ThinGate->bIsOpen);
    TestTrue(TEXT("Raised closing door still clears player"), ThinGate->AllowsExitForPlayer(Player));
    Player->SetActorLocation(FVector(0.f, 1340.f, 90.f));
    ThinExit->Tick(.3f);
    TestTrue(TEXT("Crossing thin box between frames completes exit"), ThinExit->bCompleted);
    TestFalse(TEXT("Locked final barrier cannot be bypassed by raised door"), ThinGate->AllowsExitForPlayer(Player));
    TestEqual(TEXT("Hitch transition advances Room 3 to Room 4 exactly once"), Replay->CurrentRoomNumber, 4);
    ThinExit->Tick(.3f);
    TestEqual(TEXT("Swept exit remains latched"), Replay->CurrentRoomNumber, 4);
    return true;
}
#endif
