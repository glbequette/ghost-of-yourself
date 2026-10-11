#include "../GhostPressurePlate.h"
#include "../GhostGate.h"
#include "../RecordedGhost.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostPlateGateTest, "GhostOfYourself.Puzzles.PlateGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGhostPlateGateTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Init = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false).CreateFXSystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None,
        nullptr, true, ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("Transient test world exists"), World))
    {
        return false;
    }
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };

    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AGhostPressurePlate* Plate = World->SpawnActor<AGhostPressurePlate>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    AGhostGate* Gate = World->SpawnActor<AGhostGate>(FVector(1000.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
    ACharacter* Player = World->SpawnActor<ACharacter>(FVector(500.f, 0.f, 90.f), FRotator::ZeroRotator, Spawn);
    ARecordedGhost* Ghost = World->SpawnActor<ARecordedGhost>(FVector(500.f, 0.f, 90.f), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Plate spawned"), Plate) || !TestNotNull(TEXT("Gate spawned"), Gate)
        || !TestNotNull(TEXT("Player spawned"), Player) || !TestNotNull(TEXT("Ghost spawned"), Ghost))
    {
        return false;
    }
    Plate->GateRef = Gate;
    TestFalse(TEXT("Unnumbered gate has no label"), Gate->FrontRoomNumber->IsVisible());
    Gate->SetDestinationRoomNumber(2);
    TestEqual(TEXT("Door shows only the destination number"), Gate->FrontRoomNumber->Text.ToString(), FString(TEXT("02")));
    TestEqual(TEXT("Both faces have the same number"), Gate->BackRoomNumber->Text.ToString(), Gate->FrontRoomNumber->Text.ToString());
    TestTrue(TEXT("Door label has no collision"), Gate->FrontRoomNumber->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
    Plate->UpdateOccupancy(Player, Ghost);
    TestFalse(TEXT("Empty plate closes the gate"), Gate->bIsOpen);
    TestTrue(TEXT("Closed gate is visible and collidable"), Gate->GateMesh->IsVisible()
        && Gate->GateMesh->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics);
    TestFalse(TEXT("Null actor cannot occupy plate"), Plate->IsActorOnPlate(nullptr));

    Player->SetActorLocation(FVector(0.f, 0.f, 90.f));
    Plate->UpdateOccupancy(Player, Ghost);
    TestTrue(TEXT("Live player opens the gate"), Gate->bIsOpen);
    TestTrue(TEXT("Occupied plate lights up"), Plate->ActiveLight->IsVisible());
    TestTrue(TEXT("Opening gate remains visible and collidable"),
        Gate->GateMesh->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics && Gate->GateMesh->IsVisible());
    const FVector ClosedPosition = Gate->GateMesh->GetRelativeLocation();
    const FVector ClosedLabelPosition = Gate->FrontRoomNumber->GetComponentLocation();
    const FTransform GateActorTransform = Gate->GetActorTransform();
    Gate->AdvanceMovement(0.1f);
    TestTrue(TEXT("Number moves with the opening panel"), Gate->FrontRoomNumber->GetComponentLocation().Equals(
        ClosedLabelPosition + FVector(0.f, 0.f, Gate->TravelSpeed * .1f), .01f));
    TestTrue(TEXT("Gate slides instead of jumping to its open height"),
        Gate->GateMesh->GetRelativeLocation().Equals(ClosedPosition + FVector(0.f, 0.f, Gate->TravelSpeed * 0.1f), 0.01f));
    Gate->SetOpen(false);
    Gate->AdvanceMovement(0.1f);
    TestTrue(TEXT("Mid-motion reversal returns smoothly to closed position"),
        Gate->GateMesh->GetRelativeLocation().Equals(ClosedPosition, 0.01f));
    Gate->SetOpen(true);
    Gate->AdvanceMovement(10.f);
    const FVector OpenPosition = ClosedPosition + FVector(0.f, 0.f, Gate->OpenHeight);
    TestTrue(TEXT("Large frame clamps at open height"), Gate->GateMesh->GetRelativeLocation().Equals(OpenPosition, 0.01f));
    Gate->SetOpen(true);
    Gate->AdvanceMovement(10.f);
    TestTrue(TEXT("Repeated open commands do not move the closed anchor"),
        Gate->GateMesh->GetRelativeLocation().Equals(OpenPosition, 0.01f));
    TestTrue(TEXT("Only the door mesh moves; actor origin stays fixed"), Gate->GetActorTransform().Equals(GateActorTransform));

    Ghost->SetActorLocation(FVector(0.f, 0.f, 90.f));
    Player->SetActorLocation(FVector(500.f, 0.f, 90.f));
    Plate->UpdateOccupancy(Player, Ghost);
    TestTrue(TEXT("Ghost keeps gate open after player leaves"), Gate->bIsOpen);
    TestFalse(TEXT("Ghost actor collision remains disabled"), Ghost->GetActorEnableCollision());
    TestTrue(TEXT("Plate uses no overlap collision"),
        Plate->ActivationBox->GetCollisionEnabled() == ECollisionEnabled::NoCollision);

    TArray<FRecordedGhostSample> Recording;
    FRecordedGhostSample Start;
    Start.Transform.SetLocation(FVector(500.f, 0.f, 90.f));
    Recording.Add(Start);
    FRecordedGhostSample End;
    End.Time = 1.f;
    End.Transform.SetLocation(FVector(0.f, 0.f, 90.f));
    Recording.Add(End);
    Ghost->SetRecording(Recording);
    Ghost->EvaluateAtTime(100.f);
    Plate->UpdateOccupancy(Player, Ghost);
    TestTrue(TEXT("Held replay endpoint keeps gate open"), Gate->bIsOpen);
    Ghost->EvaluateAtTime(0.f);
    Plate->UpdateOccupancy(Player, Ghost);
    TestFalse(TEXT("Restart away from plate closes gate"), Gate->bIsOpen);
    Gate->AdvanceMovement(10.f);
    TestTrue(TEXT("Restart lowers gate to its original location"),
        Gate->GateMesh->GetRelativeLocation().Equals(ClosedPosition, 0.01f));
    Ghost->EvaluateAtTime(100.f);
    Plate->UpdateOccupancy(Player, Ghost);
    Ghost->Destroy();
    Plate->UpdateOccupancy(Player, Ghost);
    TestFalse(TEXT("Destroyed ghost cannot keep gate open"), Gate->bIsOpen);
    TestFalse(TEXT("Empty plate light turns off"), Plate->ActiveLight->IsVisible());

    Plate->SetActorTransform(FTransform(FRotator(0.f, 35.f, 0.f),
        FVector(300.f, -120.f, 0.f), FVector(2.f, 0.5f, 1.f)));
    const FTransform BoxTransform = Plate->ActivationBox->GetComponentTransform();
    Player->SetActorLocation(BoxTransform.TransformPosition(FVector(99.f, 99.f, 0.f)));
    TestTrue(TEXT("Rotated nonuniformly scaled box detects an inside point"), Plate->IsActorOnPlate(Player));
    Player->SetActorLocation(BoxTransform.TransformPosition(FVector(101.f, 0.f, 0.f)));
    TestFalse(TEXT("Box scale is not counted twice"), Plate->IsActorOnPlate(Player));
    Player->SetActorLocation(BoxTransform.TransformPosition(FVector(0.f, 0.f, 100.f)));
    TestTrue(TEXT("Boundary point counts as occupied"), Plate->IsActorOnPlate(Player));

    Plate->GateRef = nullptr;
    Gate->SetDestinationRoomNumber(0);
    TestTrue(TEXT("Zero clears label and hides both faces"), Gate->FrontRoomNumber->Text.IsEmpty()
        && !Gate->FrontRoomNumber->IsVisible() && !Gate->BackRoomNumber->IsVisible());
    Plate->UpdateOccupancy(nullptr, nullptr);
    TestFalse(TEXT("Missing links and actors are safe"), Plate->bOccupied);
    return true;
}
#endif
