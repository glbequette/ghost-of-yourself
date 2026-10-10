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

// Place one in L_TestRoom. Uses the existing first-person player without reparenting it.
UCLASS(Blueprintable)
class GHOSTOFYOURSELF_API AGhostReplayController : public AActor
{
    GENERATED_BODY()

public:
    AGhostReplayController();
    virtual void Tick(float DeltaSeconds) override;

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

    // Clear the room's echo and adopt a new fixed recording start without teleporting.
    bool AdvanceToNextRoom(const ATargetPoint* NextRoomStart);

    // Native room actors clear transient state after every attempt reset.
    FSimpleMulticastDelegate OnAttemptReset;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ghost")
    int32 CurrentRoomNumber = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost", meta = (ClampMin = "0.05", ClampMax = "120"))
    float MaxRecordingSeconds = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost", meta = (ClampMin = "0.01", ClampMax = "1"))
    float SampleInterval = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost")
    TSubclassOf<ARecordedGhost> GhostClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost")
    bool bShowDebugStatus = true;

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
    float FadeElapsed = 0.f;
};
