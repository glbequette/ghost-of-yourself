#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostIntroController.generated.h"

class UAudioComponent;
class UBoxComponent;
class USoundBase;
class AGhostGate;
class APlayerController;
class AGhostReplayController;
class ATargetPoint;

// Place once in a separate intro level. The puzzle level retains its own player start and controller.
UCLASS(Blueprintable)
class GHOSTOFYOURSELF_API AGhostIntroController : public AActor
{
    GENERATED_BODY()
public:
    AGhostIntroController();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "Ghost|Intro")
    bool BeginIntroduction();

    UFUNCTION(BlueprintCallable, Category = "Ghost|Intro")
    void FinishIntroduction();

    bool CanEnterAssessment(const AActor* ActorToCheck) const;
    bool TryEnterAssessment(const AActor* ActorToCheck);
    void ResetForRoomCheat(int32 RoomNumber);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Intro")
    TObjectPtr<UAudioComponent> Narration;

    // Actor is floor-level; the box is centered one meter above it, beyond the door.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Intro")
    TObjectPtr<UBoxComponent> ExitBox;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ghost|Intro")
    TObjectPtr<AGhostGate> GateRef;

    // Assign for Room 0 in L_TestRoom. Without it, the original separate-level travel mode is used.
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ghost|Room 0")
    TObjectPtr<AGhostReplayController> ReplayControllerRef;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ghost|Room 0")
    TObjectPtr<ATargetPoint> RoomOneStart;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ghost|Room 0")
    bool bRoomZeroCompleted = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Intro")
    TObjectPtr<USoundBase> IntroductionAudio;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ghost|Intro")
    bool bUseRoomNarration = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Intro")
    FName AssessmentLevel = TEXT("/Game/GhostOfYourself/Maps/L_TestRoom");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Intro", meta = (ClampMin = "0", Units = "s"))
    float StartDelaySeconds = .75f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Intro", meta = (ClampMin = "0", ClampMax = "2", Units = "s"))
    float ExitFadeSeconds = .35f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Intro")
    bool bAllowEnterToSkip = true;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ghost|Intro")
    bool bIntroStarted = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ghost|Intro")
    bool bIntroFinished = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ghost|Intro")
    bool bTravelling = false;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void OnNarrationFinished(UAudioComponent* FinishedComponent);
    void ReleaseTravelInput();

    UPROPERTY()
    TObjectPtr<APlayerController> TravelController;

    bool bTravelInputLocked = false;
    bool bReportedMissingLevel = false;
    float DelayElapsed = 0.f;
    float NarrationElapsed = 0.f;
    float NarrationDuration = 0.f;
    float FadeElapsed = 0.f;
};
