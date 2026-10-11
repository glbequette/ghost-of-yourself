#include "../GhostIntroController.h"
#include "../GhostGate.h"
#include "../RecordedGhost.h"
#include "../GhostReplayController.h"
#include "Engine/TargetPoint.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostIntroTest, "GhostOfYourself.Intro.Flow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGhostIntroTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Init = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false).CreateFXSystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("Test world"), World)) { return false; }
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->InitializeActorsForPlay(FURL());
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AGhostIntroController* Intro = World->SpawnActor<AGhostIntroController>(FVector(0.f, 300.f, 0.f), FRotator::ZeroRotator, Spawn);
    AGhostGate* Gate = World->SpawnActor<AGhostGate>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    APlayerController* PC = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    ACharacter* Player = World->SpawnActor<ACharacter>(FVector(0.f, -300.f, 90.f), FRotator::ZeroRotator, Spawn);
    ARecordedGhost* Ghost = World->SpawnActor<ARecordedGhost>(FVector(0.f, 300.f, 90.f), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Intro actor"), Intro) || !TestNotNull(TEXT("Gate"), Gate)
        || !TestNotNull(TEXT("PC"), PC) || !TestNotNull(TEXT("Player"), Player)
        || !TestNotNull(TEXT("Ghost"), Ghost)) { return false; }
    Intro->GateRef = Gate;
    Gate->DispatchBeginPlay();
    Intro->DispatchBeginPlay();
    TestFalse(TEXT("No possession means narration waits"), Intro->BeginIntroduction());
    PC->SetAsLocalPlayerController();
    PC->Possess(Player);
    Player->SetActorLocation(FVector(0.f, 300.f, 90.f));
    TestFalse(TEXT("Doorway does not bypass unfinished intro"), Intro->CanEnterAssessment(Player));
    Intro->FinishIntroduction();
    TestTrue(TEXT("Skip finishes intro and opens gate"), Intro->bIntroStarted && Intro->bIntroFinished && Gate->bIsOpen);
    TestFalse(TEXT("Intro cannot replay after completion"), Intro->BeginIntroduction());
    TestFalse(TEXT("Null actor cannot exit"), Intro->CanEnterAssessment(nullptr));
    TestFalse(TEXT("Ghost cannot exit"), Intro->CanEnterAssessment(Ghost));
    TestTrue(TEXT("Live player beyond gate can exit"), Intro->CanEnterAssessment(Player));
    Player->SetActorLocation(FVector(0.f, -20.f, 90.f));
    Intro->ExitBox->SetBoxExtent(FVector(1000.f));
    TestFalse(TEXT("Oversized trigger cannot exit from wrong side"), Intro->CanEnterAssessment(Player));
    Player->SetActorLocation(FVector(0.f, 20.f, 90.f));
    TestFalse(TEXT("Entire capsule must clear gate"), Intro->CanEnterAssessment(Player));
    Player->SetActorLocation(FVector(0.f, 300.f, 90.f));
    Intro->ExitBox->SetBoxExtent(FVector(100.f));
    Intro->AssessmentLevel = TEXT("/Game/GhostOfYourself/Maps/NonexistentIntroTestDestination");
    AddExpectedError(TEXT("Intro destination level does not exist"), EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("Invalid destination does not travel"), Intro->TryEnterAssessment(Player));
    TestFalse(TEXT("Invalid destination leaves inputs usable"), Intro->bTravelling || PC->IsMoveInputIgnored() || PC->IsLookInputIgnored());
    Intro->AssessmentLevel = TEXT("/Game/GhostOfYourself/Maps/L_TestRoom");
    TestTrue(TEXT("Valid destination starts transition"), Intro->TryEnterAssessment(Player));
    TestTrue(TEXT("Fade locks movement and look"), PC->IsMoveInputIgnored() && PC->IsLookInputIgnored());
    TestFalse(TEXT("Travel is submitted once"), Intro->TryEnterAssessment(Player));
    Intro->Destroy(); // No Tick past fade: do not actually switch levels during the test.
    TestFalse(TEXT("Destroying intro releases its input locks"), PC->IsMoveInputIgnored() || PC->IsLookInputIgnored());
    AGhostReplayController* Replay = World->SpawnActor<AGhostReplayController>();
    AGhostIntroController* Welcome = World->SpawnActor<AGhostIntroController>(FVector(0.f, 300.f, 0.f), FRotator::ZeroRotator, Spawn);
    ATargetPoint* RoomOne = World->SpawnActor<ATargetPoint>(FVector(0.f, 600.f, 0.f), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Room 0 replay"), Replay) || !TestNotNull(TEXT("Room 0 welcome"), Welcome)
        || !TestNotNull(TEXT("Room 1 checkpoint"), RoomOne)) { return false; }
    Replay->CurrentRoomNumber = 0;
    Replay->bUseResetFade = false;
    Replay->bShowDebugStatus = false;
    Replay->Tick(0.f);
    Replay->ToggleRecording();
    TestFalse(TEXT("Recording is disabled in Room 0"), Replay->bRecording);
    Welcome->GateRef = Gate;
    Welcome->ReplayControllerRef = Replay;
    Welcome->RoomOneStart = RoomOne;
    Welcome->DispatchBeginPlay();
    TestFalse(TEXT("Room 0 exit waits for narration"), Welcome->TryEnterAssessment(Player));
    Welcome->FinishIntroduction();
    Welcome->RoomOneStart = nullptr;
    TestFalse(TEXT("Room 0 requires a Room 1 checkpoint"), Welcome->TryEnterAssessment(Player));
    Welcome->RoomOneStart = RoomOne;
    TestTrue(TEXT("Room 0 enters Room 1 in the same map"), Welcome->TryEnterAssessment(Player));
    TestEqual(TEXT("Room 0 increments to Room 1"), Replay->CurrentRoomNumber, 1);
    TestFalse(TEXT("Same-map welcome does not request level travel"), Welcome->bTravelling);
    TestTrue(TEXT("Welcome completes and locks its gate"), Welcome->bRoomZeroCompleted && Gate->bLockedClosed);
    TestFalse(TEXT("Welcome cannot advance twice"), Welcome->TryEnterAssessment(Player));
    Replay->ToggleRecording();
    TestTrue(TEXT("Recording unlocks in Room 1"), Replay->bRecording);
    TestTrue(TEXT("Recording resets to Room 1 rather than Room 0"), Player->GetActorLocation().Y > 500.f);
    return true;
}
#endif
