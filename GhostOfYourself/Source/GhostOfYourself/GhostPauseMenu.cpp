#include "GhostPauseMenu.h"
#include "GhostUserSettings.h"
#include "GhostReplayController.h"
#include "GhostLabMusic.h"
#include "GhostRoomNarration.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AudioDevice.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IInputProcessor.h"
#include "Widgets/SViewport.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

class FGhostPauseEscapeInput : public IInputProcessor
{
    TWeakObjectPtr<UGhostPauseMenu> Menu;
public:
    explicit FGhostPauseEscapeInput(UGhostPauseMenu* InMenu) : Menu(InMenu) {}
    virtual void Tick(float, FSlateApplication&, TSharedRef<ICursor>) override {}
    virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& Event) override
    {
        if (Event.GetKey() != EKeys::Escape || !Menu.IsValid()) { return false; }
        UWorld* World = Menu->GetWorld();
        UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
        const TSharedPtr<SViewport> Widget = Viewport ? Viewport->GetGameViewportWidget() : nullptr;
        if (!Widget) { return false; }
        const bool bMenuWindowActive = Menu->IsMenuOpen()
            && SlateApp.FindWidgetWindow(Widget.ToSharedRef()) == SlateApp.GetActiveTopLevelWindow();
        if (!Widget->HasAnyUserFocusOrFocusedDescendants() && !bMenuWindowActive) { return false; }
        if (Event.IsRepeat()) { return true; }
        return Menu->TogglePauseMenu();
    }
};

UGhostPauseMenu::UGhostPauseMenu()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
}

bool UGhostPauseMenu::InitializePlayer()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!IsValid(PC) || !PC->IsLocalController() || !PC->GetPawn()) { return false; }
    PlayerController = PC;
    InstallSensitivity();
    if (!EscapeHandler && FSlateApplication::IsInitialized())
    {
        EscapeHandler = MakeShared<FGhostPauseEscapeInput>(this);
        FSlateApplication::Get().RegisterInputPreProcessor(EscapeHandler, 0);
    }
    ApplySettings();
    return true;
}

void UGhostPauseMenu::InstallSensitivity()
{
    MouseLookAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_MouseLook.IA_MouseLook"));
    if (!MouseLookAction) { return; }
    for (UInputModifier* Modifier : MouseLookAction->Modifiers)
    {
        if (UGhostMouseSensitivityModifier* Existing = Cast<UGhostMouseSensitivityModifier>(Modifier))
        { SensitivityModifier = Existing; return; }
    }
    SensitivityModifier = NewObject<UGhostMouseSensitivityModifier>(MouseLookAction);
    MouseLookAction->Modifiers.Add(SensitivityModifier);
    bOwnsModifier = true;
    if (ULocalPlayer* Local = PlayerController->GetLocalPlayer())
    {
        if (auto* Subsystem = Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        { Subsystem->RequestRebuildControlMappings(FModifyContextOptions(), EInputMappingRebuildType::RebuildWithFlush); }
    }
}

void UGhostPauseMenu::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(DeltaSeconds, TickType, ThisTick);
    if (!IsValid(PlayerController)) { InitializePlayer(); }
}

void UGhostPauseMenu::ApplySettings()
{
    GetMutableDefault<UGhostUserSettings>()->ClampValues();
    if (GetWorld())
    {
        FAudioDeviceHandle Device = GetWorld()->GetAudioDevice();
        if (Device.IsValid()) { Device->SetTransientPrimaryVolume(GetDefault<UGhostUserSettings>()->MasterVolume); }
    }
    if (AGhostReplayController* Replay = Cast<AGhostReplayController>(GetOwner()))
    {
        if (Replay->LabMusic) { Replay->LabMusic->RefreshSettingsVolume(); }
        if (Replay->RoomNarration) { Replay->RoomNarration->SetVolumeMultiplier(GetDefault<UGhostUserSettings>()->NarratorVolume); }
    }
}

float UGhostPauseMenu::GetSetting(int32 Index) const
{
    const UGhostUserSettings* Settings = GetDefault<UGhostUserSettings>();
    switch (Index)
    {
    case 0: return (Settings->MouseSensitivity - .1f) / 2.9f;
    case 1: return Settings->MusicVolume;
    case 2: return Settings->NarratorVolume;
    default: return Settings->MasterVolume;
    }
}

void UGhostPauseMenu::SetSetting(int32 Index, float Value)
{
    Value = FMath::Clamp(Value, 0.f, 1.f);
    UGhostUserSettings* Settings = GetMutableDefault<UGhostUserSettings>();
    switch (Index)
    {
    case 0: Settings->MouseSensitivity = FMath::Lerp(.1f, 3.f, Value); break;
    case 1: Settings->MusicVolume = Value; break;
    case 2: Settings->NarratorVolume = Value; break;
    case 3: Settings->MasterVolume = Value; break;
    default: return;
    }
    ApplySettings();
}

bool UGhostPauseMenu::TogglePauseMenu()
{
    if (bMenuOpen) { return ResumeGame(); }
    if (!InitializePlayer()) { return false; }
    const AGhostReplayController* Replay = Cast<AGhostReplayController>(GetOwner());
    if (!Replay || Replay->bGameCompleted || UGameplayStatics::IsGamePaused(this)) { return false; }
    UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
    if (!Viewport || !UGameplayStatics::SetGamePaused(this, true)) { return false; }
    PreviousCapture = Viewport->GetMouseCaptureMode();
    PreviousLock = Viewport->GetMouseLockMode();
    bPreviousCursor = PlayerController->bShowMouseCursor;
    PlayerController->SetIgnoreMoveInput(true);
    PlayerController->SetIgnoreLookInput(true);
    bInputLocked = true;
    bMenuOpen = true;
    PlayerController->bShowMouseCursor = true;
    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PlayerController->SetInputMode(Mode);
    Viewport->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
    Viewport->SetMouseLockMode(EMouseLockMode::DoNotLock);
    ShowPage(false);
    return true;
}

bool UGhostPauseMenu::ResumeGame()
{
    if (!bMenuOpen) { return false; }
    if (MenuWidget && GetWorld() && GetWorld()->GetGameViewport())
    { GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(MenuWidget.ToSharedRef()); }
    MenuWidget.Reset();
    bMenuOpen = false;
    UGameplayStatics::SetGamePaused(this, false);
    if (IsValid(PlayerController))
    {
        if (bInputLocked)
        { PlayerController->SetIgnoreMoveInput(false); PlayerController->SetIgnoreLookInput(false); bInputLocked = false; }
        PlayerController->bShowMouseCursor = bPreviousCursor;
        PlayerController->SetInputMode(FInputModeGameOnly());
        if (GetWorld()->GetGameViewport())
        {
            GetWorld()->GetGameViewport()->SetMouseCaptureMode(PreviousCapture);
            GetWorld()->GetGameViewport()->SetMouseLockMode(PreviousLock);
        }
    }
    GetMutableDefault<UGhostUserSettings>()->SaveConfig();
    return true;
}

void UGhostPauseMenu::ShowPage(bool bSettings)
{
    UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
    if (MenuWidget) { Viewport->RemoveViewportWidgetContent(MenuWidget.ToSharedRef()); }
    MenuWidget = CreateMenuWidget(bSettings);
    Viewport->AddViewportWidgetContent(MenuWidget.ToSharedRef(), 200);
}

FReply UGhostPauseMenu::ContinueClicked() { ResumeGame(); return FReply::Handled(); }
FReply UGhostPauseMenu::SettingsClicked() { ShowPage(true); return FReply::Handled(); }
FReply UGhostPauseMenu::BackClicked() { GetMutableDefault<UGhostUserSettings>()->SaveConfig(); ShowPage(false); return FReply::Handled(); }
FReply UGhostPauseMenu::QuitClicked()
{
    GetMutableDefault<UGhostUserSettings>()->SaveConfig();
    UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
    return FReply::Handled();
}

TSharedRef<SWidget> UGhostPauseMenu::SettingsRow(const FString& Label, int32 Index)
{
    const TWeakObjectPtr<UGhostPauseMenu> Weak(this);
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
        [ SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.f)
            [ SNew(STextBlock).Text(FText::FromString(Label)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)) ]
            + SHorizontalBox::Slot().AutoWidth()
            [ SNew(STextBlock).Text_Lambda([Weak, Index]()
                {
                    if (!Weak.IsValid()) { return FText::GetEmpty(); }
                    if (Index == 0) { return FText::FromString(FString::Printf(TEXT("%.2fx"), FMath::Lerp(.1f, 3.f, Weak->GetSetting(0)))); }
                    return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Weak->GetSetting(Index) * 100.f)));
                }) ] ]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 22.f)
        [ SNew(SSlider).Value_Lambda([Weak, Index]() { return Weak.IsValid() ? Weak->GetSetting(Index) : 0.f; })
            .OnValueChanged_Lambda([Weak, Index](float Value) { if (Weak.IsValid()) { Weak->SetSetting(Index, Value); } })
            .OnMouseCaptureEnd_Lambda([]() { GetMutableDefault<UGhostUserSettings>()->SaveConfig(); }) ];
}

TSharedRef<SWidget> UGhostPauseMenu::CreateMenuWidget(bool bSettings)
{
    const TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);
    Content->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 26.f)
        [ SNew(STextBlock).Text(FText::FromString(bSettings ? TEXT("SETTINGS") : TEXT("PAUSED")))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 26)) ];
    auto AddButton = [&Content, this](const TCHAR* Label, FReply (UGhostPauseMenu::*Callback)())
    {
        Content->AddSlot().AutoHeight().Padding(0.f, 6.f)
        [ SNew(SBox).HeightOverride(54.f)
            [ SNew(SButton).IsFocusable(false).HAlign(HAlign_Center).VAlign(VAlign_Center)
                .ButtonColorAndOpacity(FLinearColor(.08f, .22f, .28f, 1.f))
                .OnClicked(FOnClicked::CreateUObject(this, Callback))
                [ SNew(STextBlock).Text(FText::FromString(Label)).Font(FCoreStyle::GetDefaultFontStyle("Bold", 19))
                    .ColorAndOpacity(FLinearColor::White) ] ] ];
    };
    if (bSettings)
    {
        const TCHAR* Labels[] = { TEXT("Mouse sensitivity"), TEXT("Music volume"), TEXT("Narrator volume"), TEXT("Master volume") };
        for (int32 Index = 0; Index < 4; ++Index) { Content->AddSlot().AutoHeight()[SettingsRow(Labels[Index], Index)]; }
        AddButton(TEXT("BACK"), &UGhostPauseMenu::BackClicked);
    }
    else
    {
        AddButton(TEXT("CONTINUE"), &UGhostPauseMenu::ContinueClicked);
        AddButton(TEXT("SETTINGS"), &UGhostPauseMenu::SettingsClicked);
        AddButton(TEXT("QUIT"), &UGhostPauseMenu::QuitClicked);
    }
    return SNew(SOverlay)
        + SOverlay::Slot()
        [ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, .78f)) ]
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
        [ SNew(SBox).WidthOverride(500.f)
            [ SNew(SBorder).Padding(30.f).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .ForegroundColor(FLinearColor::White)
                .BorderBackgroundColor(FLinearColor(.025f, .055f, .07f, 1.f))[Content] ] ];
}

void UGhostPauseMenu::EndPlay(const EEndPlayReason::Type Reason)
{
    if (bMenuOpen) { ResumeGame(); }
    if (EscapeHandler && FSlateApplication::IsInitialized())
    { FSlateApplication::Get().UnregisterInputPreProcessor(EscapeHandler); }
    EscapeHandler.Reset();
    if (bOwnsModifier && MouseLookAction)
    {
        MouseLookAction->Modifiers.Remove(SensitivityModifier);
        if (PlayerController && PlayerController->GetLocalPlayer())
        {
            if (auto* Subsystem = PlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            { Subsystem->RequestRebuildControlMappings(FModifyContextOptions(), EInputMappingRebuildType::RebuildWithFlush); }
        }
    }
    if (GetWorld())
    {
        FAudioDeviceHandle Device = GetWorld()->GetAudioDevice();
        if (Device.IsValid()) { Device->SetTransientPrimaryVolume(1.f); }
    }
    Super::EndPlay(Reason);
}
