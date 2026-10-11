#include "../GhostSecurityBeam.h"
#include "../GhostReplayController.h"
#include "../GhostGate.h"
#include "../RecordedGhost.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostSecurityBeamTest, "GhostOfYourself.Puzzles.SecurityBeam",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGhostSecurityBeamTest::RunTest(const FString& Parameters)
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
    ACharacter* Player = World->SpawnActor<ACharacter>(FVector(250.f, -200.f, 88.f), FRotator::ZeroRotator, Spawn);
    ARecordedGhost* Ghost = World->SpawnActor<ARecordedGhost>(FVector(100.f, 0.f, 88.f), FRotator::ZeroRotator, Spawn);
    AGhostSecurityBeam* Beam = World->SpawnActor<AGhostSecurityBeam>(FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Replay"), Replay) || !TestNotNull(TEXT("Player"), Player)
        || !TestNotNull(TEXT("PC"), PC) || !TestNotNull(TEXT("Ghost"), Ghost)
        || !TestNotNull(TEXT("Beam"), Beam)) { return false; }
    PC->SetAsLocalPlayerController();
    PC->Possess(Player);
    Replay->CurrentRoomNumber = 3;
    Replay->bUseResetFade = false;
    Replay->bShowDebugStatus = false;
    Replay->Tick(0.f);
    Beam->ControllerRef = Replay;
    AGhostGate* Gate = World->SpawnActor<AGhostGate>(FVector(1000.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Gate"), Gate)) { return false; }
    Gate->DispatchBeginPlay();
    Beam->GateRef = Gate;
    Beam->DispatchBeginPlay();
    TestTrue(TEXT("Virtual ghost capsule blocks"), Beam->IntersectsCapsule(Ghost->GetActorLocation(), 34.f, 88.f));
    TestFalse(TEXT("Distant ghost does not block"), Beam->IntersectsCapsule(FVector(100.f, 200.f, 88.f), 34.f, 88.f));
    TestFalse(TEXT("Ghost beyond beam endpoint does not block"), Beam->IntersectsCapsule(FVector(1300.f, 0.f, 88.f), 34.f, 88.f));
    TestFalse(TEXT("Ghost above beam does not block"), Beam->IntersectsCapsule(FVector(100.f, 0.f, 400.f), 34.f, 88.f));
    TestTrue(TEXT("Fast crossing detected"), Beam->CrossedBeam(FVector(250.f, -200.f, 88.f), FVector(250.f, 200.f, 88.f), 34.f, 88.f));
    TestFalse(TEXT("Parallel motion misses"), Beam->CrossedBeam(FVector(0.f, 200.f, 88.f), FVector(500.f, 200.f, 88.f), 34.f, 88.f));
    TestEqual(TEXT("Beam has no physical collision"), Beam->BeamMesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
    TestTrue(TEXT("Default beam spans 12 meters"), Beam->BeamEnd->GetRelativeLocation().Equals(FVector(1200.f, 0.f, 0.f)));
    float BlockDistance = -1.f;
    TestTrue(TEXT("Find first ghost surface contact"), Beam->FindGhostBlockDistance(Ghost->GetActorLocation(), 34.f, 88.f, BlockDistance));
    TestTrue(TEXT("Beam stops at near capsule surface"), FMath::IsNearlyEqual(BlockDistance, 64.f, .01f));
    TestTrue(TEXT("Emitter inside ghost blocks immediately"), Beam->FindGhostBlockDistance(FVector(0.f, 0.f, 88.f), 34.f, 88.f, BlockDistance));
    TestTrue(TEXT("Immediate contact has zero length"), FMath::IsNearlyZero(BlockDistance));
    TestFalse(TEXT("Off-axis ghost does not shorten beam"), Beam->FindGhostBlockDistance(FVector(100.f, 200.f, 88.f), 34.f, 88.f, BlockDistance));
    Replay->GhostRef = Ghost;
    Beam->Tick(0.f);
    TestTrue(TEXT("Ghost truncates beam"), Beam->bBlockedByGhost);
    TestTrue(TEXT("Emitter side stays dangerous"), Beam->bHazardActive);
    TestTrue(TEXT("Impact glow appears"), Beam->ImpactGlow->IsVisible());
    TestTrue(TEXT("Impact glow sits at ghost contact"), FMath::IsNearlyEqual(Beam->ImpactGlow->GetRelativeLocation().X, 64.f, .01f));
    TestEqual(TEXT("Impact glow uses beam material"), Beam->ImpactGlow->GetMaterial(0), Beam->BeamMesh->GetMaterial(0));
    TestEqual(TEXT("Impact glow has no physical collision"), Beam->ImpactGlow->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
    TestTrue(TEXT("Blocked beam remains visible to ghost"), Beam->BeamMesh->IsVisible());
    TestTrue(TEXT("Visible beam ends at ghost surface"), FMath::IsNearlyEqual(Beam->BeamMesh->GetRelativeLocation().X * 2.f, 64.f, .01f));
    TestTrue(TEXT("Ghost opens linked gate"), Gate->bIsOpen);
    Player->SetActorLocation(FVector(250.f, 200.f, 88.f));
    Beam->Tick(0.f);
    TestTrue(TEXT("Player crosses safely while ghost blocks"), Player->GetActorLocation().Equals(FVector(250.f, 200.f, 88.f)));
    Player->SetActorLocation(FVector(20.f, 0.f, 88.f));
    Beam->Tick(0.f);
    TestTrue(TEXT("Touching emitter side resets player"), Player->GetActorLocation().Equals(FVector(250.f, -200.f, 88.f)));
    TestEqual(TEXT("Wrong-side reset preserves ghost"), Replay->GhostRef.Get(), Ghost);
    Beam->Tick(0.f);
    Player->SetActorLocation(FVector(20.f, -200.f, 88.f));
    Beam->Tick(0.f);
    Player->SetActorLocation(FVector(20.f, 200.f, 88.f));
    Beam->Tick(0.f);
    TestTrue(TEXT("Fast emitter-side crossing resets player"), Player->GetActorLocation().Equals(FVector(250.f, -200.f, 88.f)));
    Player->SetActorLocation(FVector(250.f, -200.f, 88.f));
    Beam->Tick(0.f);
    Ghost->SetActorLocation(FVector(100.f, 200.f, 88.f));
    Beam->Tick(0.f);
    TestTrue(TEXT("Leaving beam reactivates hazard"), Beam->bHazardActive);
    TestFalse(TEXT("Leaving beam hides impact glow"), Beam->ImpactGlow->IsVisible());
    TestTrue(TEXT("Leaving beam restores full visual length"), FMath::IsNearlyEqual(Beam->BeamMesh->GetRelativeScale3D().Z, 12.f));
    TestFalse(TEXT("Unblocked beam closes linked gate"), Gate->bIsOpen);
    Player->SetActorLocation(FVector(250.f, 200.f, 88.f));
    Beam->Tick(0.f);
    TestTrue(TEXT("Crossing restarts at checkpoint"), Player->GetActorLocation().Equals(FVector(250.f, -200.f, 88.f)));
    TestEqual(TEXT("Failure preserves saved ghost"), Replay->GhostRef.Get(), Ghost);
    Replay->bRecording = true;
    Player->SetActorLocation(FVector(250.f, 0.f, 88.f));
    Beam->Tick(0.f);
    TestFalse(TEXT("Recording is safe"), Beam->bHazardActive);
    TestTrue(TEXT("Recording not canceled by beam"), Replay->bRecording);
    Replay->bRecording = false;
    Replay->bResetInProgress = true;
    Beam->Tick(0.f);
    TestFalse(TEXT("Fade reset is safe"), Beam->bHazardActive);
    Replay->bResetInProgress = false;
    Replay->CurrentRoomNumber = 4;
    Beam->Tick(0.f);
    TestFalse(TEXT("Previous room beam disabled"), Beam->bHazardActive);
    TestFalse(TEXT("Previous room beam hidden"), Beam->BeamMesh->IsVisible());
    TestFalse(TEXT("Previous room impact hidden"), Beam->ImpactGlow->IsVisible());
    Beam->SetActorRotation(FRotator(0.f, 90.f, 0.f));
    TestTrue(TEXT("Rotated beam blocks in world space"), Beam->IntersectsCapsule(FVector(0.f, 250.f, 88.f), 34.f, 88.f));
    TestTrue(TEXT("Rotated beam finds first contact"), Beam->FindGhostBlockDistance(FVector(0.f, 250.f, 88.f), 34.f, 88.f, BlockDistance));
    TestTrue(TEXT("Rotated beam surface distance"), FMath::IsNearlyEqual(BlockDistance, 214.f, .01f));
    TestFalse(TEXT("Rotated beam rejects old direction"), Beam->IntersectsCapsule(FVector(250.f, 0.f, 88.f), 34.f, 88.f));
    return true;
}
#endif
