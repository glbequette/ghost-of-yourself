#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostRecording.h"
#include "GhostReplayController.generated.h"

class ACharacter;
class APlayerController;
class ARecordedGhost;
class ATargetPoint;
class AGhostButton;
class AHUD;
class UCanvas;
class UTexture2D;
class UGhostRoomNarration;
class UGhostLabMusic;
class UGhostPauseMenu;

USTRUCT()
struct FAdditionalGhostReplay
{
    GENERATED_BODY()
    UPROPERTY() TObjectPtr<ARecordedGhost> Ghost;
    UPROPERTY() TArray<FRecordedGhostSample> Samples;
    UPROPERTY() TArray<FRecordedGhostButtonPress> ButtonPresses;
    int32 NextButtonEvent = 0;
};

// Place one in L_TestRoom. Uses the existing first-person player without reparenting it.
UCLASS(Blueprintable)
class GHOSTOFYOURSELF_API AGhostReplayController : public AActor
{
    GENERATED_BODY()

public:
    AGhostReplayController();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ghost|Narration")
    TObjectPtr<UGhostRoomNarration> RoomNarration;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ghost|Music")
    TObjectPtr<UGhostLabMusic> LabMusic;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ghost|Menu")
    TObjectPtr<UGhostPauseMenu> PauseMenu;
    virtual void Tick(float DeltaSeconds) override;
    virtual void PostRenderFor(APlayerController* PC, UCanvas* Canvas, FVector CameraPosition, FVector CameraDir) override;
    static float WispOpacityAtUV(const FVector2D& UV);
    float GetRecordingWispBlend() const { return RecordingWispBlend; }
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|HUD", meta=(DisplayName="Show Recording Wisps"))
    bool bShowPhaseHUD = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|HUD", meta=(ClampMin="0", ClampMax="1"))
    float RecordingWispStrength = .8f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|HUD")
    bool bShowControlsHUD = true;
    TArray<FString> GetControlHints() const;
    FString GetGhostCounterText() const;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Movement", meta=(ClampMin="0", Units="cm/s"))
    float PlayerMaxWalkSpeed = 400.f;

    UFUNCTION(BlueprintCallable, Category = "Ghost")
    void ToggleRecording();

    UFUNCTION(BlueprintCallable, Category = "Ghost")
    void CancelOrRestart();

    UFUNCTION(BlueprintCallable, Category = "Ghost")
    void UndoGhost();

    // Input selection checks view and visibility; this entry point validates the linked room and range.
    bool TryPressButton(AGhostButton* Button);

    UPROPERTY()
    TArray<FRecordedGhostButtonPress> SavedButtonPresses;

    // Slot 1 keeps its existing fields for compatibility with placed actors and native puzzles.
    UPROPERTY() TArray<FAdditionalGhostReplay> AdditionalGhosts;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Multiple Ghosts", meta = (ClampMin = "0"))
    int32 DefaultGhostLimit = 1;
    // Room number -> saved ghost slots. Unlisted rooms use Default Ghost Limit.
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ghost|Multiple Ghosts")
    TMap<int32, int32> RoomGhostLimits;
    int32 GetGhostLimit() const;
    int32 GetSavedGhostCount() const;
    void GetGhostActors(TArray<ARecordedGhost*>& OutGhosts, bool bIncludeSuppressed = false) const;

    // Clear the room's echo and adopt a new fixed recording start without teleporting.
    bool AdvanceToNextRoom(const ATargetPoint* NextRoomStart);

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Ghost|Completion")
    bool bGameCompleted = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ghost|Completion")
    FName CompletionTitleLevel = TEXT("/Game/GhostOfYourself/Maps/L_Title");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ghost|Completion", meta=(ClampMin="0.5", Units="s"))
    float CompletionReturnDelay = .75f;
    bool IsCompletionNarrationFinished() const;

    // Compatibility entry point; developer room jumping has been removed.
    bool JumpToRoom(int32 RoomNumber);

    // Native room actors clear transient state after every attempt reset.
    FSimpleMulticastDelegate OnAttemptReset;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ghost", meta = (ClampMin = "0"))
    int32 CurrentRoomNumber = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost", meta = (ClampMin = "0.05", ClampMax = "120"))
    float MaxRecordingSeconds = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost", meta = (ClampMin = "0.01", ClampMax = "1"))
    float SampleInterval = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost")
    TSubclassOf<ARecordedGhost> GhostClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost")
    bool bShowDebugStatus = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Transition")
    bool bUseResetFade = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Transition", meta = (ClampMin = "0", ClampMax = "1", Units = "s"))
    float FadeOutSeconds = 0.12f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Transition", meta = (ClampMin = "0", ClampMax = "1", Units = "s"))
    float FadeInSeconds = 0.18f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ghost|Transition")
    bool bResetInProgress = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ghost")
    bool bRecording = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ghost")
    float Clock = 0.f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ghost")
    TObjectPtr<ARecordedGhost> GhostRef;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ghost")
    TArray<FRecordedGhostSample> SavedSamples;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    bool TryInitializePlayer();
    void CaptureSample();
    AGhostButton* FindButtonToPress() const;
    void ReplayButtonEvents();
    void ClearSavedGhosts();
    void RemoveOldestGhost();
    int32 FindFreeEchoNumber() const;
    void RestoreSuppressedGhost();
    void UpdateHUDRegistration();
    void RemoveHUDRegistration();
    void CreateWispTexture();
    void DrawControlHints(AHUD* HUD, UCanvas* Canvas) const;
    void CommitRecording();
    void ResetAttempt(bool bStartRecording = false);
    void PerformReset();
    void UpdateResetTransition(float DeltaSeconds);
    void FinishResetTransition();
    void ReleaseResetInput();
    void ShowStatus() const;
    void Notify(const FString& Message) const;

    UPROPERTY()
    TObjectPtr<ACharacter> PlayerRef;

    UPROPERTY()
    TObjectPtr<APlayerController> ControllerRef;

    UPROPERTY()
    TArray<FRecordedGhostSample> WorkingSamples;

    UPROPERTY()
    TArray<FRecordedGhostButtonPress> WorkingButtonPresses;

    int32 NextButtonEvent = 0;

    FTransform StartTransform = FTransform::Identity;
    FRotator StartControlRotation = FRotator::ZeroRotator;
    float NextSampleTime = 0.f;
    bool bInitialized = false;
    bool bFadingIn = false;
    bool bRecordAfterReset = false;
    bool bResetInputLocked = false;
    TWeakObjectPtr<AHUD> RegisteredHUD;
    bool bPreviousShowOverlays = false;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> RecordingWispTexture;
    float RecordingWispBlend = 0.f;
    float WispAnimationTime = 0.f;
    TWeakObjectPtr<ARecordedGhost> SuppressedGhost;
    bool bSuppressedGhostWasHidden = false;
    float FadeElapsed = 0.f;
    bool bCompletionInputLocked = false;
    bool bCompletionFading = false;
    bool bTitleTravelRequested = false;
    float CompletionElapsed = 0.f;
};
