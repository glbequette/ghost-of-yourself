#include "../GhostPauseMenu.h"
#include "../GhostUserSettings.h"
#include "../GhostReplayController.h"
#include "../GhostLabMusic.h"
#include "../GhostRoomNarration.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerState.h"
#include "Widgets/SWidget.h"
#include "Layout/Children.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostPauseMenuTest, "GhostOfYourself.UI.PauseSettings",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGhostPauseMenuTest::RunTest(const FString& Parameters)
{
    UGhostUserSettings* Settings = GetMutableDefault<UGhostUserSettings>();
    const float OldSensitivity = Settings->MouseSensitivity, OldMusic = Settings->MusicVolume,
        OldNarrator = Settings->NarratorVolume, OldMaster = Settings->MasterVolume;
    ON_SCOPE_EXIT
    {
        Settings->MouseSensitivity = OldSensitivity; Settings->MusicVolume = OldMusic;
        Settings->NarratorVolume = OldNarrator; Settings->MasterVolume = OldMaster;
    };
    const UWorld::InitializationValues Init = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false).CreateFXSystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None,
        nullptr, true, ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("World"), World)) { return false; }
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->InitializeActorsForPlay(FURL());
    AGhostReplayController* Replay = World->SpawnActor<AGhostReplayController>();
    if (!TestNotNull(TEXT("Replay"), Replay)) { return false; }
    UGhostPauseMenu* Menu = Replay->PauseMenu;
    if (!TestNotNull(TEXT("Automatic pause component"), Menu)) { return false; }
    auto CountWidgets = [](const TSharedRef<SWidget>& Root, const FName Type)
    {
        int32 Count = 0;
        TFunction<void(const TSharedRef<SWidget>&)> Visit;
        Visit = [&Count, &Visit, Type](const TSharedRef<SWidget>& Widget)
        {
            if (Widget->GetType() == Type) { ++Count; }
            FChildren* Children = Widget->GetChildren();
            for (int32 Index = 0; Children && Index < Children->Num(); ++Index) { Visit(Children->GetChildAt(Index)); }
        };
        Visit(Root);
        return Count;
    };
    TestEqual(TEXT("Pause page has three standard buttons"), CountWidgets(Menu->CreateMenuWidget(false), TEXT("SButton")), 3);
    TestEqual(TEXT("Settings page has four real sliders"), CountWidgets(Menu->CreateMenuWidget(true), TEXT("SSlider")), 4);
    Menu->SetSetting(0, 0.f);
    TestEqual(TEXT("Minimum sensitivity"), Settings->MouseSensitivity, .1f);
    Menu->SetSetting(0, 1.f);
    TestEqual(TEXT("Maximum sensitivity"), Settings->MouseSensitivity, 3.f);
    Settings->MouseSensitivity = 2.f;
    UGhostMouseSensitivityModifier* Modifier = NewObject<UGhostMouseSensitivityModifier>();
    const FInputActionValue Scaled = Modifier->ModifyRaw_Implementation(nullptr, FInputActionValue(FVector2D(2.f, -3.f)), 0.f);
    TestTrue(TEXT("Mouse look scales both axes and preserves sign"), Scaled.Get<FVector2D>().Equals(FVector2D(4.f, -6.f)));
    Menu->SetSetting(1, 0.f);
    TestEqual(TEXT("Music can be muted independently"), Replay->LabMusic->GetTargetMusicVolume(), 0.f);
    Menu->SetSetting(1, .5f);
    TestTrue(TEXT("Music slider scales ambient volume"), FMath::IsNearlyEqual(Replay->LabMusic->GetTargetMusicVolume(), Replay->LabMusic->AmbientVolume * .5f));
    Menu->SetSetting(2, .3f);
    TestTrue(TEXT("Narrator volume applies independently"), FMath::IsNearlyEqual(Replay->RoomNarration->VolumeMultiplier, .3f));
    Menu->SetSetting(3, -1.f);
    TestEqual(TEXT("Master volume clamps at zero"), Settings->MasterVolume, 0.f);
    Menu->SetSetting(3, 2.f);
    TestEqual(TEXT("Master volume clamps at one"), Settings->MasterVolume, 1.f);
    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    APlayerController* PC = World->SpawnActor<APlayerController>();
    ACharacter* Player = World->SpawnActor<ACharacter>();
    if (!TestNotNull(TEXT("Pauser"), Pauser) || !TestNotNull(TEXT("Player controller"), PC)
        || !TestNotNull(TEXT("Player"), Player)) { return false; }
    PC->SetAsLocalPlayerController();
    PC->Possess(Player);
    Replay->bShowControlsHUD = Replay->bShowPhaseHUD = false;
    Replay->Tick(0.f);
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    TestTrue(TEXT("World is actually paused"), UGameplayStatics::IsGamePaused(Replay));
    Replay->Clock = 4.f;
    Replay->bRecording = true;
    Replay->Tick(5.f);
    TestEqual(TEXT("Pause holds recording clock even if explicitly ticked"), Replay->Clock, 4.f);
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    return true;
}
#endif
