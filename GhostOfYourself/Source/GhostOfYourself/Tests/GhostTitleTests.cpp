#include "../GhostTitleScreen.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Widgets/SWidget.h"
#include "Widgets/Input/SButton.h"
#include "Layout/Children.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostTitleTest, "GhostOfYourself.Title.Flow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGhostTitleTest::RunTest(const FString& Parameters)
{
    const AGhostTitleGameMode* Mode = GetDefault<AGhostTitleGameMode>();
    TestTrue(TEXT("Title does not spawn playable character"), Mode->DefaultPawnClass == nullptr);
    TestTrue(TEXT("Title uses native HUD/controller"), Mode->HUDClass == AGhostTitleHUD::StaticClass()
        && Mode->PlayerControllerClass == AGhostTitlePlayerController::StaticClass());
    const UWorld::InitializationValues Init = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false).CreateFXSystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("Transient world"), World)) { return false; }
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->InitializeActorsForPlay(FURL());
    AGhostTitlePlayerController* PC = World->SpawnActor<AGhostTitlePlayerController>();
    if (!TestNotNull(TEXT("Menu controller"), PC)) { return false; }
    PC->SetAsLocalPlayerController();
    const TSharedRef<SWidget> Menu = PC->CreateMenuWidget();
    TestEqual(TEXT("Native UI is created independently of canvas hit boxes"), Menu->GetTypeAsString(), FString(TEXT("SOverlay")));
    TArray<TSharedRef<SButton>> Buttons;
    TFunction<void(const TSharedRef<SWidget>&)> FindButtons;
    FindButtons = [&Buttons, &FindButtons](const TSharedRef<SWidget>& Widget)
    {
        if (Widget->GetType() == FName(TEXT("SButton")))
        {
            Buttons.Add(StaticCastSharedRef<SButton>(Widget));
        }
        FChildren* Children = Widget->GetChildren();
        for (int32 Index = 0; Children && Index < Children->Num(); ++Index)
        {
            FindButtons(Children->GetChildAt(Index));
        }
    };
    FindButtons(Menu);
    if (!TestEqual(TEXT("Menu has real Begin and Quit buttons"), Buttons.Num(), 2)) { return false; }
    TestFalse(TEXT("Begin does not accept keyboard focus"), Buttons[0]->SupportsKeyboardFocus());
    TestFalse(TEXT("Quit does not accept keyboard focus"), Buttons[1]->SupportsKeyboardFocus());
    TestEqual(TEXT("BEGIN skips the pending introduction"), PC->IntroductionLevel,
        FName(TEXT("/Game/GhostOfYourself/Maps/L_TestRoom")));
    PC->IntroductionLevel = TEXT("/Game/GhostOfYourself/Maps/NonexistentTitleTestDestination");
    AddExpectedError(TEXT("Title destination level does not exist"), EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("Missing intro stays in menu"), PC->BeginGame());
    TestFalse(TEXT("Missing intro cannot latch transition"), PC->bBeginningGame);
    TestTrue(TEXT("Missing intro gives visible feedback"), !PC->MenuError.IsEmpty());
    // Use the existing puzzle map for package validation; L_Intro is created after this build.
    PC->IntroductionLevel = TEXT("/Game/GhostOfYourself/Maps/L_TestRoom");
    PC->FadeSeconds = 2.f;
    Buttons[0]->SimulateClick();
    TestTrue(TEXT("Valid destination starts fade"), PC->bBeginningGame);
    TestTrue(TEXT("Success clears setup error"), PC->MenuError.IsEmpty());
    TestFalse(TEXT("Cursor and mouse clicks disabled during travel"), PC->bShowMouseCursor || PC->bEnableClickEvents);
    TestFalse(TEXT("Double click cannot submit second transition"), PC->BeginGame());
    PC->Tick(.5f);
    TestTrue(TEXT("Fade advances smoothly"), FMath::IsNearlyEqual(PC->GetTransitionAlpha(), .25f));
    PC->Tick(-1.f);
    TestTrue(TEXT("Negative time cannot rewind fade"), FMath::IsNearlyEqual(PC->GetTransitionAlpha(), .25f));
    // Do not tick past the fade or call Quit: leave the automation session in its test world.
    return true;
}
#endif

