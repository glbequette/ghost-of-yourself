#include "../GhostReplayController.h"
#include "../GhostButtonGroup.h"
#include "../GhostButton.h"
#include "../GhostGate.h"
#include "../GhostPressurePlate.h"
#include "../GhostSecurityBeam.h"
#include "../RecordedGhost.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostMultipleGhostTest, "GhostOfYourself.Recording.MultipleGhosts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGhostMultipleGhostTest::RunTest(const FString& Parameters)
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
    AGhostGate* Gate = World->SpawnActor<AGhostGate>(FVector(3000.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
    AGhostButtonGroup* Group = World->SpawnActor<AGhostButtonGroup>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    AGhostButton* A = World->SpawnActor<AGhostButton>(FVector(0.f, 0.f, 120.f), FRotator::ZeroRotator, Spawn);
    AGhostButton* B = World->SpawnActor<AGhostButton>(FVector(1000.f, 0.f, 120.f), FRotator::ZeroRotator, Spawn);
    AGhostButton* C = World->SpawnActor<AGhostButton>(FVector(2000.f, 0.f, 120.f), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Replay"), Replay) || !TestNotNull(TEXT("PC"), PC)
        || !TestNotNull(TEXT("Player"), Player) || !TestNotNull(TEXT("Gate"), Gate)
        || !TestNotNull(TEXT("Group"), Group) || !TestNotNull(TEXT("A"), A)
        || !TestNotNull(TEXT("B"), B) || !TestNotNull(TEXT("C"), C)) { return false; }
    PC->SetAsLocalPlayerController();
    PC->Possess(Player);
    Replay->bUseResetFade = false;
    Replay->bShowDebugStatus = false;
    Replay->CurrentRoomNumber = 5;
    TestEqual(TEXT("Unlisted rooms use default one slot"), Replay->GetGhostLimit(), 1);
    Replay->RoomGhostLimits.Add(5, 4);
    TestEqual(TEXT("Any room can have four slots"), Replay->GetGhostLimit(), 4);
    Replay->RoomGhostLimits.Add(5, 0);
    TestEqual(TEXT("Room can disable saving ghosts"), Replay->GetGhostLimit(), 0);
    Replay->RoomGhostLimits.Remove(5);
    Replay->DefaultGhostLimit = 2;
    TestEqual(TEXT("Unlisted rooms follow editable default"), Replay->GetGhostLimit(), 2);
    Replay->DefaultGhostLimit = 1;
    Replay->CurrentRoomNumber = 6;
    TestEqual(TEXT("Room 6 explicit default entry has two slots"), Replay->GetGhostLimit(), 2);
    Replay->Tick(0.f);
    Group->ControllerRef = Replay;
    Group->GateRef = Gate;
    Group->RoomNumber = 6;
    Group->bShowDebugStatus = false;
    A->GroupRef = B->GroupRef = C->GroupRef = Group;
    Gate->DispatchBeginPlay();
    Group->DispatchBeginPlay();
    Replay->ToggleRecording();
    Player->SetActorLocation(FVector(500.f, 0.f, 90.f));
    Replay->Tick(1.f);
    Player->SetActorLocation(FVector(0.f, 0.f, 90.f));
    Replay->Tick(3.f);
    TestTrue(TEXT("First recording captures A"), Replay->TryPressButton(A));
    Replay->Tick(1.f);
    Replay->ToggleRecording();
    if (!TestNotNull(TEXT("First ghost"), Replay->GhostRef.Get())) { return false; }
    ARecordedGhost* First = Replay->GhostRef.Get();
    TestEqual(TEXT("First ghost is blue slot"), First->EchoNumber, 1);
    Replay->ToggleRecording();
    TestTrue(TEXT("Second recording starts without removing first"), Replay->bRecording && Replay->GhostRef == First);
    Player->SetActorLocation(FVector(1000.f, 0.f, 90.f));
    Replay->Tick(1.f);
    TestTrue(TEXT("First ghost moves during second recording"), First->GetActorLocation().Equals(FVector(500.f, 0.f, 90.f), .01f));
    Replay->CancelOrRestart();
    TestEqual(TEXT("Canceling second recording preserves first"), Replay->GetSavedGhostCount(), 1);
    Replay->ToggleRecording();
    Player->SetActorLocation(FVector(1000.f, 0.f, 90.f));
    Replay->Tick(1.f);
    Replay->Tick(3.f);
    TestTrue(TEXT("Second recording captures B"), Replay->TryPressButton(B));
    TestFalse(TEXT("Two presses do not unlock three buttons"), Group->bUnlocked);
    Replay->Tick(1.f);
    Replay->ToggleRecording();
    TestEqual(TEXT("Two tracks saved"), Replay->GetSavedGhostCount(), 2);
    if (Replay->AdditionalGhosts.Num() != 1 || !TestNotNull(TEXT("Second ghost"), Replay->AdditionalGhosts[0].Ghost.Get())) { return false; }
    ARecordedGhost* Second = Replay->AdditionalGhosts[0].Ghost.Get();
    TestEqual(TEXT("Second ghost is yellow slot"), Second->EchoNumber, 2);
    TestEqual(TEXT("First ghost preserved"), Replay->GhostRef.Get(), First);
    TestFalse(TEXT("Saving rewinds and resets group"), Group->bUnlocked);
    Replay->ToggleRecording();
    TestTrue(TEXT("Full limit allows replacement recording"), Replay->bRecording);
    TestEqual(TEXT("Replacement does not retire ghosts before saving"), Replay->GetSavedGhostCount(), 2);
    Replay->CancelOrRestart();
    TestEqual(TEXT("Cancel preserves full set of ghosts"), Replay->GetSavedGhostCount(), 2);
    Player->SetActorLocation(FVector(2000.f, 0.f, 90.f));
    Replay->Tick(4.f);
    TestFalse(TEXT("Two ghosts wait for live third button"), Group->bUnlocked);
    TestTrue(TEXT("Player presses third button"), Replay->TryPressButton(C));
    TestTrue(TEXT("Three presses open gate"), Group->bUnlocked && Gate->bIsOpen);
    // Earlier event in slot 2 must be handled before slot 1 when a frame crosses both.
    Replay->AdditionalGhosts[0].ButtonPresses[0].Time = 3.f;
    Replay->CancelOrRestart();
    Player->SetActorLocation(FVector(2000.f, 0.f, 90.f));
    Replay->Tick(4.f);
    Replay->TryPressButton(C);
    TestTrue(TEXT("Tracks merge button events chronologically"), Group->bUnlocked);
    C->GroupRef = nullptr;
    Replay->AdditionalGhosts[0].ButtonPresses[0].Time = 1.f;
    Replay->CancelOrRestart();
    Replay->Tick(4.f);
    TestFalse(TEXT("Slow frame cannot combine widely separated presses from different tracks"), Group->bUnlocked);
    C->GroupRef = Group;
    Replay->AdditionalGhosts[0].ButtonPresses[0].Time = 3.f;
    Replay->CancelOrRestart();
    Replay->Tick(6.f);
    TestTrue(TEXT("Both ghosts hold endpoints"), First->GetActorLocation().Equals(FVector(0.f, 0.f, 90.f))
        && Second->GetActorLocation().Equals(FVector(1000.f, 0.f, 90.f)));
    // Existing puzzle actors must also recognize slot 2.
    AGhostPressurePlate* Plate = World->SpawnActor<AGhostPressurePlate>(FVector(1000.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
    AGhostSecurityBeam* Beam = World->SpawnActor<AGhostSecurityBeam>(FVector(800.f, 0.f, 100.f), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Plate"), Plate) || !TestNotNull(TEXT("Beam"), Beam)) { return false; }
    Plate->ControllerRef = Replay;
    Plate->GateRef = Gate;
    Plate->DispatchBeginPlay();
    Beam->ControllerRef = Replay;
    Beam->RoomNumber = 6;
    Beam->DispatchBeginPlay();
    Player->SetActorLocation(FVector(2000.f, 200.f, 90.f));
    Plate->Tick(0.f);
    TestTrue(TEXT("Second ghost activates plate"), Plate->bOccupied);
    Beam->Tick(0.f);
    TestTrue(TEXT("Second ghost blocks beam"), Beam->bBlockedByGhost);
    TestTrue(TEXT("Beam stops at second ghost"), FMath::IsNearlyEqual(Beam->BeamMesh->GetRelativeLocation().X * 2.f, 164.f, .01f));
    Replay->UndoGhost();
    TestEqual(TEXT("Undo removes newest only"), Replay->GetSavedGhostCount(), 1);
    TestEqual(TEXT("Undo preserves blue ghost"), Replay->GhostRef.Get(), First);
    Replay->RoomGhostLimits.Add(6, 3);
    Replay->ToggleRecording();
    Replay->Tick(20.f);
    TestFalse(TEXT("Second recording automatically saves at 20 seconds"), Replay->bRecording);
    TestEqual(TEXT("Automatic save preserves first and adds second"), Replay->GetSavedGhostCount(), 2);
    TestEqual(TEXT("Automatic save never replaces first"), Replay->GhostRef.Get(), First);
    Replay->ToggleRecording();
    Replay->Tick(1.f);
    Replay->ToggleRecording();
    TestEqual(TEXT("Optional three-slot limit"), Replay->GetSavedGhostCount(), 3);
    if (Replay->AdditionalGhosts.Num() == 2 && IsValid(Replay->AdditionalGhosts[1].Ghost))
    {
        TestEqual(TEXT("Third ghost uses green slot"), Replay->AdditionalGhosts[1].Ghost->EchoNumber, 3);
    }
    else { AddError(TEXT("Third ghost missing")); }
    Replay->RoomGhostLimits.Add(6, 4);
    Replay->ToggleRecording();
    Replay->Tick(1.f);
    Replay->ToggleRecording();
    TestEqual(TEXT("Room limit supports more than three ghosts"), Replay->GetSavedGhostCount(), 4);
    if (Replay->AdditionalGhosts.Num() == 3 && IsValid(Replay->AdditionalGhosts[2].Ghost))
    {
        TestEqual(TEXT("Fourth ghost keeps slot number four"), Replay->AdditionalGhosts[2].Ghost->EchoNumber, 4);
    }
    TestTrue(TEXT("Transition clears all echoes"), Replay->AdvanceToNextRoom(nullptr));
    TestEqual(TEXT("No saved tracks after transition"), Replay->GetSavedGhostCount(), 0);
    TestTrue(TEXT("No ghosts after transition"), Replay->GhostRef == nullptr && Replay->AdditionalGhosts.IsEmpty());
    return true;
}
#endif
