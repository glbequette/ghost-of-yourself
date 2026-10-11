#include "../GhostPressurePlateGroup.h"
#include "../GhostPressurePlate.h"
#include "../GhostReplayController.h"
#include "../GhostGate.h"
#include "../RecordedGhost.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostPlateGroupTest, "GhostOfYourself.Puzzles.PlateGroup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGhostPlateGroupTest::RunTest(const FString& Parameters)
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
    ACharacter* Player = World->SpawnActor<ACharacter>(FVector(1000.f, 200.f, 88.f), FRotator::ZeroRotator, Spawn);
    ARecordedGhost* First = World->SpawnActor<ARecordedGhost>(FVector(0.f, 0.f, 88.f), FRotator::ZeroRotator, Spawn);
    ARecordedGhost* Second = World->SpawnActor<ARecordedGhost>(FVector(500.f, 0.f, 88.f), FRotator::ZeroRotator, Spawn);
    AGhostGate* Gate = World->SpawnActor<AGhostGate>(FVector(2000.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
    AGhostPressurePlateGroup* Group = World->SpawnActor<AGhostPressurePlateGroup>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    TArray<AGhostPressurePlate*> Plates;
    for (int32 Index = 0; Index < 3; ++Index)
    {
        AGhostPressurePlate* Plate = World->SpawnActor<AGhostPressurePlate>(FVector(Index * 500.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
        if (!TestNotNull(TEXT("Plate"), Plate)) { return false; }
        Plate->GroupRef = Group;
        Plates.Add(Plate);
    }
    if (!TestNotNull(TEXT("Replay"), Replay) || !TestNotNull(TEXT("Player"), Player)
        || !TestNotNull(TEXT("PC"), PC) || !TestNotNull(TEXT("First ghost"), First)
        || !TestNotNull(TEXT("Second ghost"), Second) || !TestNotNull(TEXT("Gate"), Gate)
        || !TestNotNull(TEXT("Group"), Group)) { return false; }
    PC->SetAsLocalPlayerController();
    PC->Possess(Player);
    Replay->CurrentRoomNumber = 6;
    Replay->GhostRef = First;
    Replay->AdditionalGhosts.AddDefaulted_GetRef().Ghost = Second;
    Group->ControllerRef = Replay;
    Group->GateRef = Gate;
    Group->RoomNumber = 6;
    Gate->DispatchBeginPlay();
    Group->DispatchBeginPlay();
    // Even a stale per-plate gate assignment cannot compete with the group.
    Plates[0]->GateRef = Gate;
    Plates[0]->UpdateCurrentOccupancy();
    TestTrue(TEXT("Grouped plate inherits controller and recognizes ghost"), Plates[0]->bOccupied);
    TestFalse(TEXT("One grouped plate cannot independently open gate"), Gate->bIsOpen);
    Group->Tick(0.f);
    TestEqual(TEXT("Two ghosts occupy two plates"), Group->OccupiedPlateCount, 2);
    TestFalse(TEXT("Missing player plate keeps gate closed"), Group->bAllOccupied || Gate->bIsOpen);
    Player->SetActorLocation(FVector(1000.f, 0.f, 88.f));
    Group->Tick(0.f);
    TestTrue(TEXT("All three occupants open gate"), Group->bAllOccupied && Gate->bIsOpen);
    Second->SetActorLocation(FVector(500.f, 200.f, 88.f));
    Group->Tick(0.f);
    TestFalse(TEXT("Releasing one plate closes gate"), Group->bAllOccupied || Gate->bIsOpen);
    Second->SetActorLocation(FVector(500.f, 0.f, 88.f));
    Replay->bResetInProgress = true;
    Group->Tick(0.f);
    TestFalse(TEXT("Fade keeps gate closed"), Gate->bIsOpen);
    Replay->bResetInProgress = false;
    Replay->CurrentRoomNumber = 7;
    Group->Tick(0.f);
    TestFalse(TEXT("Wrong room cannot open gate"), Gate->bIsOpen);
    Replay->CurrentRoomNumber = 6;
    AGhostPressurePlate* Fourth = World->SpawnActor<AGhostPressurePlate>(FVector(1500.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
    ARecordedGhost* Third = World->SpawnActor<ARecordedGhost>(FVector(1500.f, 0.f, 88.f), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Fourth plate"), Fourth) || !TestNotNull(TEXT("Third ghost"), Third)) { return false; }
    Fourth->GroupRef = Group;
    Plates.Add(Fourth);
    Group->Tick(0.f);
    TestFalse(TEXT("Newly linked fourth plate becomes required"), Gate->bIsOpen);
    Replay->AdditionalGhosts.AddDefaulted_GetRef().Ghost = Third;
    Group->Tick(0.f);
    TestEqual(TEXT("Group has no three-plate cap"), Group->OccupiedPlateCount, 4);
    TestTrue(TEXT("Four occupants open four-plate gate"), Gate->bIsOpen);
    Gate->LockClosed();
    Group->Tick(0.f);
    TestFalse(TEXT("Completed gate remains locked"), Gate->bIsOpen);
    Gate->ResetForRoomCheat();
    for (AGhostPressurePlate* Plate : Plates) { Plate->GroupRef = nullptr; }
    Group->Tick(0.f);
    TestEqual(TEXT("Empty group has no members"), Group->TotalPlateCount, 0);
    TestFalse(TEXT("Empty group never opens gate"), Group->bAllOccupied || Gate->bIsOpen);
    return true;
}
#endif
