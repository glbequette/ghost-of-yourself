#include "GhostTitleScreen.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

DEFINE_LOG_CATEGORY_STATIC(LogGhostTitle, Log, All);

AGhostTitleGameMode::AGhostTitleGameMode()
{
    DefaultPawnClass = nullptr;
    bStartPlayersAsSpectators = true;
    SpectatorClass = nullptr;
    HUDClass = AGhostTitleHUD::StaticClass();
    PlayerControllerClass = AGhostTitlePlayerController::StaticClass();
}

AGhostTitlePlayerController::AGhostTitlePlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

void AGhostTitlePlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController()) { return; }
    UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
    if (Viewport)
    {
        PreviousCaptureMode = Viewport->GetMouseCaptureMode();
        PreviousLockMode = Viewport->GetMouseLockMode();
        bSavedViewportSettings = true;
    }
    FInputModeGameAndUI Mode;
    if (Viewport)
    {
        MenuWidget = CreateMenuWidget();
        Viewport->AddViewportWidgetContent(MenuWidget.ToSharedRef(), 100);
    }
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
    if (Viewport)
    {
        Viewport->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
        Viewport->SetMouseLockMode(EMouseLockMode::DoNotLock);
    }
}

bool AGhostTitlePlayerController::BeginGame()
{
    if (bBeginningGame || !IsLocalController()) { return false; }
    if (!FPackageName::IsValidLongPackageName(IntroductionLevel.ToString())
        || !FPackageName::DoesPackageExist(IntroductionLevel.ToString()))
    {
        MenuError = TEXT("Starting level is missing. Run the title-screen setup script.");
        UE_LOG(LogGhostTitle, Error, TEXT("Title destination level does not exist: %s"), *IntroductionLevel.ToString());
        return false;
    }
    MenuError.Reset();
    bBeginningGame = true;
    FadeElapsed = 0.f;
    bShowMouseCursor = false;
    bEnableClickEvents = false;
    if (MenuWidget) { MenuWidget->SetVisibility(EVisibility::Hidden); }
    return true;
}

float AGhostTitlePlayerController::GetTransitionAlpha() const
{
    return bBeginningGame ? (FadeSeconds > UE_SMALL_NUMBER
        ? FMath::Clamp(FadeElapsed / FadeSeconds, 0.f, 1.f) : 1.f) : 0.f;
}

void AGhostTitlePlayerController::QuitFromMenu()
{
    if (!bBeginningGame && IsLocalController())
    {
        UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
    }
}

void AGhostTitlePlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bBeginningGame) { return; }
    FadeElapsed += FMath::Max(0.f, DeltaSeconds);
    if (GetTransitionAlpha() >= 1.f)
    {
        SetActorTickEnabled(false);
        SetInputMode(FInputModeGameOnly());
        UGameplayStatics::OpenLevel(this, IntroductionLevel);
    }
}

FReply AGhostTitlePlayerController::OnBeginButtonClicked()
{
    BeginGame();
    return FReply::Handled();
}

FReply AGhostTitlePlayerController::OnQuitButtonClicked()
{
    QuitFromMenu();
    return FReply::Handled();
}

TSharedRef<SWidget> AGhostTitlePlayerController::CreateMenuWidget()
{
    // Standard Slate buttons own hit testing, DPI scaling, mouse capture and
    // click dispatch. The canvas HUD only draws the artwork behind them.
    return SNew(SOverlay)
        .Visibility(EVisibility::SelfHitTestInvisible)
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(16.f, 0.f, 16.f, 36.f))
        [
            SNew(SBox).WidthOverride(340.f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 16.f)
                [
                    SNew(SBox).HeightOverride(64.f)
                    [
                        SNew(SButton).IsFocusable(false)
                        .HAlign(HAlign_Center).VAlign(VAlign_Center)
                        .ButtonColorAndOpacity(FLinearColor(.08f, .22f, .28f, 1.f))
                        .OnClicked(FOnClicked::CreateUObject(this, &AGhostTitlePlayerController::OnBeginButtonClicked))
                        [ SNew(STextBlock).Text(FText::FromString(TEXT("BEGIN")))
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
                            .ColorAndOpacity(FLinearColor::White) ]
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SBox).HeightOverride(64.f)
                    [
                        SNew(SButton).IsFocusable(false)
                        .HAlign(HAlign_Center).VAlign(VAlign_Center)
                        .ButtonColorAndOpacity(FLinearColor(.04f, .09f, .12f, 1.f))
                        .OnClicked(FOnClicked::CreateUObject(this, &AGhostTitlePlayerController::OnQuitButtonClicked))
                        [ SNew(STextBlock).Text(FText::FromString(TEXT("QUIT")))
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
                            .ColorAndOpacity(FLinearColor::White) ]
                    ]
                ]
            ]
        ];
}

void AGhostTitlePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (MenuWidget && GetWorld() && GetWorld()->GetGameViewport())
    {
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());
    }
    MenuWidget.Reset();
    if (bSavedViewportSettings && GetWorld() && GetWorld()->GetGameViewport())
    {
        GetWorld()->GetGameViewport()->SetMouseCaptureMode(PreviousCaptureMode);
        GetWorld()->GetGameViewport()->SetMouseLockMode(PreviousLockMode);
    }
    Super::EndPlay(EndPlayReason);
}

void AGhostTitleHUD::BeginPlay()
{
    Super::BeginPlay();
    if (!LogoTexture)
    {
        LogoTexture = LoadObject<UTexture2D>(nullptr,
            TEXT("/Game/GhostOfYourself/UI/Title/T_GhostTitleLogo.T_GhostTitleLogo"), nullptr, LOAD_NoWarn);
    }
    if (!BackgroundTexture)
    {
        BackgroundTexture = LoadObject<UTexture2D>(nullptr,
            TEXT("/Game/GhostOfYourself/UI/Title/T_GhostTitleBackground.T_GhostTitleBackground"), nullptr, LOAD_NoWarn);
    }
}

void AGhostTitleHUD::DrawHUD()
{
    Super::DrawHUD();
    AGhostTitlePlayerController* PC = Cast<AGhostTitlePlayerController>(PlayerOwner);
    if (!Canvas || !PC || !GEngine || Canvas->SizeX <= 0 || Canvas->SizeY <= 0) { return; }
    const FVector2D Screen(Canvas->SizeX, Canvas->SizeY);
    const float Scale = FMath::Min(Screen.X / 1920.f, Screen.Y / 1080.f);
    DrawRect(FLinearColor(.015f, .025f, .035f), 0.f, 0.f, Screen.X, Screen.Y);
    if (IsValid(BackgroundTexture))
    {
        const float Cover = FMath::Max(Screen.X / BackgroundTexture->GetSizeX(), Screen.Y / BackgroundTexture->GetSizeY());
        const FVector2D Size(BackgroundTexture->GetSizeX() * Cover, BackgroundTexture->GetSizeY() * Cover);
        DrawTexture(BackgroundTexture, (Screen.X - Size.X) * .5f, (Screen.Y - Size.Y) * .5f,
            Size.X, Size.Y, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Opaque);
    }
    if (IsValid(LogoTexture) && LogoTexture->GetSizeX() > 0)
    {
        const float Width = FMath::Min(FMath::Min(1100.f * Scale, Screen.X * .84f),
            Screen.Y * .48f * LogoTexture->GetSizeX() / FMath::Max(1, LogoTexture->GetSizeY()));
        const float Height = Width * LogoTexture->GetSizeY() / LogoTexture->GetSizeX();
        DrawTexture(LogoTexture, (Screen.X - Width) * .5f, Screen.Y * .085f,
            Width, Height, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Translucent);
    }
    else
    {
        DrawText(TEXT("GHOST OF YOURSELF"), FLinearColor::White, Screen.X * .25f, Screen.Y * .2f, GEngine->GetLargeFont(), Scale);
    }
    if (!PC->MenuError.IsEmpty())
    {
        DrawText(PC->MenuError, FLinearColor(1.f, .6f, .5f), Screen.X * .1f, Screen.Y * .95f, GEngine->GetSmallFont(), Scale);
    }
    const float Fade = PC->GetTransitionAlpha();
    if (Fade > 0.f) { DrawRect(FLinearColor(0.f, 0.f, 0.f, Fade), 0.f, 0.f, Screen.X, Screen.Y); }
}


