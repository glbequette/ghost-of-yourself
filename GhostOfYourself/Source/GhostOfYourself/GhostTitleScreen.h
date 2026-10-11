#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "GhostTitleScreen.generated.h"

class UTexture2D;
class SWidget;
class FReply;

UCLASS()
class GHOSTOFYOURSELF_API AGhostTitleGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AGhostTitleGameMode();
};

UCLASS()
class GHOSTOFYOURSELF_API AGhostTitlePlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    AGhostTitlePlayerController();
    virtual void Tick(float DeltaSeconds) override;
    FReply OnBeginButtonClicked();
    FReply OnQuitButtonClicked();
    TSharedRef<SWidget> CreateMenuWidget();
    bool BeginGame();
    void QuitFromMenu();
    float GetTransitionAlpha() const;

    UPROPERTY(EditDefaultsOnly, Category = "Ghost|Title", meta = (DisplayName = "Begin Destination"))
    FName IntroductionLevel = TEXT("/Game/GhostOfYourself/Maps/L_TestRoom");
    UPROPERTY(EditDefaultsOnly, Category = "Ghost|Title", meta = (ClampMin = "0", ClampMax = "2"))
    float FadeSeconds = .25f;
    UPROPERTY(VisibleInstanceOnly, Category = "Ghost|Title")
    bool bBeginningGame = false;
    FString MenuError;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    float FadeElapsed = 0.f;
    TSharedPtr<SWidget> MenuWidget;
    bool bSavedViewportSettings = false;
    EMouseCaptureMode PreviousCaptureMode = EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown;
    EMouseLockMode PreviousLockMode = EMouseLockMode::LockOnCapture;
};

UCLASS()
class GHOSTOFYOURSELF_API AGhostTitleHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    UPROPERTY(EditDefaultsOnly, Category = "Ghost|Title")
    TObjectPtr<UTexture2D> LogoTexture;
    UPROPERTY(EditDefaultsOnly, Category = "Ghost|Title")
    TObjectPtr<UTexture2D> BackgroundTexture;
protected:
    virtual void BeginPlay() override;
};
