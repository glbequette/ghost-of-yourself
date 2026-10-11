#include "../GhostReplayController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostPhaseHUDTest, "GhostOfYourself.UI.PhaseHUD",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGhostPhaseHUDTest::RunTest(const FString& Parameters)
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
    if (!TestNotNull(TEXT("Replay"), Replay) || !TestNotNull(TEXT("PC"), PC) || !TestNotNull(TEXT("Player"), Player)) { return false; }
    PC->SetAsLocalPlayerController();
    PC->Possess(Player);
    PC->ClientSetHUD_Implementation(AHUD::StaticClass());
    AHUD* HUD = PC->GetHUD();
    if (!TestNotNull(TEXT("Existing gameplay HUD"), HUD)) { return false; }
    HUD->bShowOverlays = false;
    Replay->bUseResetFade = false;
    Replay->bShowDebugStatus = false;
    Replay->CurrentRoomNumber = 6;
    Replay->DispatchBeginPlay();
    Replay->Tick(0.f);
    TestEqual(TEXT("Player walking cap is 400"), Player->GetCharacterMovement()->MaxWalkSpeed, 400.f);
    Player->GetCharacterMovement()->MaxWalkSpeed = 900.f;
    Replay->Tick(0.f);
    TestEqual(TEXT("Walking cap persists during play"), Player->GetCharacterMovement()->MaxWalkSpeed, 400.f);
    TestTrue(TEXT("Controls show record/save key"), Replay->GetControlHints().Contains(FString(TEXT("R  Record / save ghost"))));
    TestEqual(TEXT("Counter uses current room allowance"), Replay->GetGhostCounterText(), FString(TEXT("Ghosts: 0 / 2")));
    Replay->CurrentRoomNumber = 0;
    TestEqual(TEXT("Welcome room shows zero allowance"), Replay->GetGhostCounterText(), FString(TEXT("Ghosts: 0 / 0")));
    TestFalse(TEXT("Welcome room does not advertise recording"), Replay->GetControlHints().Contains(FString(TEXT("R  Record / save ghost"))));
    Replay->CurrentRoomNumber = 6;
    TestTrue(TEXT("Controller attaches phase display to existing HUD"), HUD->PostRenderedActors.Contains(Replay));
    TestTrue(TEXT("Overlay drawing enabled"), HUD->bShowOverlays);
    Replay->Tick(0.f);
    TestEqual(TEXT("Repeated ticks do not register duplicate panels"), HUD->PostRenderedActors.Num(), 1);
    TestTrue(TEXT("Live play has clear edges"), FMath::IsNearlyZero(Replay->GetRecordingWispBlend()));
    TestTrue(TEXT("Centre of wisp texture is transparent"), FMath::IsNearlyZero(AGhostReplayController::WispOpacityAtUV(FVector2D(.5, .5))));
    TestTrue(TEXT("Inner play area stays transparent"), FMath::IsNearlyZero(AGhostReplayController::WispOpacityAtUV(FVector2D(.25, .25))));
    TestTrue(TEXT("Outer edge contains wisps"), AGhostReplayController::WispOpacityAtUV(FVector2D(0., .5)) > 0.f);
    Replay->ToggleRecording();
    Replay->Tick(2.5f);
    TestTrue(TEXT("Recording fades wisps in"), Replay->GetRecordingWispBlend() > .99f);
    Replay->ToggleRecording();
    Replay->Tick(.1f);
    TestEqual(TEXT("Saving updates used count"), Replay->GetGhostCounterText(), FString(TEXT("Ghosts: 1 / 2")));
    TestTrue(TEXT("Saving gently fades wisps out"), Replay->GetRecordingWispBlend() > 0.f && Replay->GetRecordingWispBlend() < 1.f);
    Replay->Tick(.5f);
    TestTrue(TEXT("Replaying ghost does not tint live player's view"), FMath::IsNearlyZero(Replay->GetRecordingWispBlend()));
    Replay->ToggleRecording();
    Replay->Tick(.2f);
    TestTrue(TEXT("Second recording brings wisps back"), Replay->GetRecordingWispBlend() > .99f);
    Replay->CancelOrRestart();
    TestEqual(TEXT("Cancel preserves saved count"), Replay->GetGhostCounterText(), FString(TEXT("Ghosts: 1 / 2")));
    Replay->Tick(.5f);
    TestTrue(TEXT("Cancel returns to clear live view"), FMath::IsNearlyZero(Replay->GetRecordingWispBlend()));
    Replay->bResetInProgress = true;
    Replay->Tick(.1f);
    TestTrue(TEXT("Reset does not add recording effect"), FMath::IsNearlyZero(Replay->GetRecordingWispBlend()));
    Replay->bResetInProgress = false;
    Replay->bShowPhaseHUD = false;
    Replay->Tick(0.f);
    TestTrue(TEXT("Controls remain available with wisps disabled"), HUD->PostRenderedActors.Contains(Replay));
    Replay->bShowControlsHUD = false;
    Replay->Tick(0.f);
    TestFalse(TEXT("Disabling both overlays unregisters"), HUD->PostRenderedActors.Contains(Replay));
    TestFalse(TEXT("Original overlay setting restored"), HUD->bShowOverlays);
    Replay->bShowPhaseHUD = true;
    Replay->Tick(0.f);
    PC->ClientSetHUD_Implementation(AHUD::StaticClass());
    AHUD* Replacement = PC->GetHUD();
    if (!TestNotNull(TEXT("Replacement HUD"), Replacement)) { return false; }
    Replay->Tick(0.f);
    TestTrue(TEXT("HUD replacement gets phase overlay"), Replacement->PostRenderedActors.Contains(Replay));
    Replay->Destroy();
    TestFalse(TEXT("Destroy removes overlay registration"), Replacement->PostRenderedActors.Contains(Replay));
    return true;
}
#endif
