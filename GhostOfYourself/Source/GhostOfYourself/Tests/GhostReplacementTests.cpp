#include "../GhostReplayController.h"
#include "../GhostButtonGroup.h"
#include "../GhostButton.h"
#include "../GhostGate.h"
#include "../RecordedGhost.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostReplacementTest, "GhostOfYourself.Recording.ReplaceOldest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGhostReplacementTest::RunTest(const FString& Parameters)
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
    if (!TestNotNull(TEXT("Replay"), Replay) || !TestNotNull(TEXT("PC"), PC)
        || !TestNotNull(TEXT("Player"), Player) || !TestNotNull(TEXT("Gate"), Gate)
        || !TestNotNull(TEXT("Group"), Group)) { return false; }
    PC->SetAsLocalPlayerController();
    PC->Possess(Player);
    Replay->bUseResetFade = false;
    Replay->bShowPhaseHUD = false;
    Replay->CurrentRoomNumber = 6;
    Replay->Tick(0.f);
    Group->ControllerRef = Replay;
    Group->GateRef = Gate;
    Group->RoomNumber = 6;
    Group->SyncWindowSeconds = .25f;
    Group->bShowDebugStatus = false;
    TArray<AGhostButton*> Buttons;
    for (int32 Index = 0; Index < 3; ++Index)
    {
        AGhostButton* Button = World->SpawnActor<AGhostButton>(FVector(Index * 500.f, 0.f, 90.f), FRotator::ZeroRotator, Spawn);
        if (!TestNotNull(TEXT("Button"), Button)) { return false; }
        Button->GroupRef = Group;
        Buttons.Add(Button);
    }
    Gate->DispatchBeginPlay();
    Group->DispatchBeginPlay();
    const auto Record = [&](int32 ButtonIndex, float Duration)
    {
        Replay->ToggleRecording();
        Player->SetActorLocation(Buttons[ButtonIndex]->GetActorLocation());
        Replay->Tick(Duration);
        TestTrue(TEXT("Record exact button event"), Replay->TryPressButton(Buttons[ButtonIndex]));
        Replay->ToggleRecording();
    };
    Record(0, 1.f);
    ARecordedGhost* First = Replay->GhostRef.Get();
    Record(1, 2.f);
    if (Replay->AdditionalGhosts.Num() != 1) { AddError(TEXT("Second recording missing")); return false; }
    ARecordedGhost* Second = Replay->AdditionalGhosts[0].Ghost.Get();
    if (!TestNotNull(TEXT("First"), First) || !TestNotNull(TEXT("Second"), Second)) { return false; }
    const int32 SecondColor = Second->EchoNumber;
    Replay->ToggleRecording();
    TestTrue(TEXT("At capacity R starts recording"), Replay->bRecording);
    TestTrue(TEXT("Oldest ghost hidden during replacement recording"), First->IsHidden());
    TestFalse(TEXT("Newer ghost stays visible"), Second->IsHidden());
    TArray<ARecordedGhost*> ActiveGhosts;
    Replay->GetGhostActors(ActiveGhosts);
    TestEqual(TEXT("Only newer ghost participates in puzzles"), ActiveGhosts.Num(), 1);
    TestTrue(TEXT("Suppressed ghost excluded from plate and beam queries"), !ActiveGhosts.Contains(First) && ActiveGhosts.Contains(Second));
    Replay->Tick(1.1f);
    Player->SetActorLocation(Buttons[1]->GetActorLocation());
    Replay->TryPressButton(Buttons[1]);
    Player->SetActorLocation(Buttons[2]->GetActorLocation());
    Replay->TryPressButton(Buttons[2]);
    TestFalse(TEXT("Hidden ghost does not replay its button event"), Group->bUnlocked);
    Replay->CancelOrRestart();
    TestEqual(TEXT("Cancel preserves oldest"), Replay->GhostRef.Get(), First);
    TestEqual(TEXT("Cancel preserves newer"), Replay->AdditionalGhosts[0].Ghost.Get(), Second);
    TestFalse(TEXT("Cancel restores oldest visibility"), First->IsHidden());
    Replay->GetGhostActors(ActiveGhosts);
    TestEqual(TEXT("Cancel restores both active ghosts"), ActiveGhosts.Num(), 2);
    Record(2, 3.f);
    TestEqual(TEXT("Replacement stays within allowance"), Replay->GetSavedGhostCount(), 2);
    TestTrue(TEXT("Oldest actor retired"), First->IsActorBeingDestroyed());
    TestEqual(TEXT("Newer actor promoted without recreation"), Replay->GhostRef.Get(), Second);
    TestEqual(TEXT("Surviving ghost keeps its color"), Second->EchoNumber, SecondColor);
    if (Replay->SavedButtonPresses.Num() != 1 || Replay->AdditionalGhosts.Num() != 1
        || Replay->AdditionalGhosts[0].ButtonPresses.Num() != 1)
    { AddError(TEXT("Replacement tracks/events missing")); return false; }
    TestEqual(TEXT("Promoted track retains exact button"), Replay->SavedButtonPresses[0].Button.Get(), Buttons[1]);
    TestEqual(TEXT("New track retains new button"), Replay->AdditionalGhosts[0].ButtonPresses[0].Button.Get(), Buttons[2]);
    ARecordedGhost* Third = Replay->AdditionalGhosts[0].Ghost.Get();
    if (!TestNotNull(TEXT("Replacement ghost"), Third)) { return false; }
    TestTrue(TEXT("Replacement uses distinct free color slot"), Third->EchoNumber != Second->EchoNumber);
    TestFalse(TEXT("Promoted surviving ghost remains visible"), Second->IsHidden());
    TestFalse(TEXT("New replacement ghost is visible"), Third->IsHidden());
    Replay->ToggleRecording();
    Replay->Tick(20.f);
    TestFalse(TEXT("Automatic replacement saves at limit"), Replay->bRecording);
    TestEqual(TEXT("Automatic replacement stays within allowance"), Replay->GetSavedGhostCount(), 2);
    TestEqual(TEXT("Automatic replacement retires next oldest"), Replay->GhostRef.Get(), Third);
    TestTrue(TEXT("Next oldest actor retired"), Second->IsActorBeingDestroyed());
    TestTrue(TEXT("Old button events not retained by new track"), Replay->AdditionalGhosts[0].ButtonPresses.IsEmpty());
    Replay->UndoGhost();
    TestEqual(TEXT("Undo still removes newest"), Replay->GetSavedGhostCount(), 1);
    TestEqual(TEXT("Undo keeps surviving track"), Replay->GhostRef.Get(), Third);
    Replay->RoomGhostLimits.Add(6, 0);
    Replay->ToggleRecording();
    TestFalse(TEXT("Zero allowance does not start replacement"), Replay->bRecording);
    TestEqual(TEXT("Zero allowance preserves existing track"), Replay->GhostRef.Get(), Third);
    // One-slot rooms should replace their only ghost too.
    Replay->RoomGhostLimits.Add(6, 1);
    Record(0, 1.f);
    TestEqual(TEXT("One-slot replacement preserves limit"), Replay->GetSavedGhostCount(), 1);
    TestTrue(TEXT("One-slot replacement retires existing ghost"), Third->IsActorBeingDestroyed());
    return true;
}
#endif
