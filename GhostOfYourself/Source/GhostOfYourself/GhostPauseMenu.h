#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineBaseTypes.h"
#include "GhostPauseMenu.generated.h"

class APlayerController;
class IInputProcessor;
class SWidget;
class FReply;
class UInputAction;
class UGhostMouseSensitivityModifier;

UCLASS()
class GHOSTOFYOURSELF_API UGhostPauseMenu : public UActorComponent
{
    GENERATED_BODY()
public:
    UGhostPauseMenu();
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTick) override;
    bool TogglePauseMenu();
    bool ResumeGame();
    bool IsMenuOpen() const { return bMenuOpen; }
    void ApplySettings();
    void SetSetting(int32 Index, float SliderValue);
    float GetSetting(int32 Index) const;
    TSharedRef<SWidget> CreateMenuWidget(bool bSettings);
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    bool InitializePlayer();
    void InstallSensitivity();
    void ShowPage(bool bSettings);
    FReply ContinueClicked();
    FReply SettingsClicked();
    FReply BackClicked();
    FReply QuitClicked();
    TSharedRef<SWidget> SettingsRow(const FString& Label, int32 Index);
    UPROPERTY() TObjectPtr<APlayerController> PlayerController;
    UPROPERTY() TObjectPtr<UInputAction> MouseLookAction;
    UPROPERTY() TObjectPtr<UGhostMouseSensitivityModifier> SensitivityModifier;
    TSharedPtr<SWidget> MenuWidget;
    TSharedPtr<IInputProcessor> EscapeHandler;
    bool bMenuOpen = false;
    bool bInputLocked = false;
    bool bPreviousCursor = false;
    bool bOwnsModifier = false;
    EMouseCaptureMode PreviousCapture = EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown;
    EMouseLockMode PreviousLock = EMouseLockMode::LockOnCapture;
};
