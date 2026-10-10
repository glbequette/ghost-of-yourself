#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostButtonGroup.generated.h"

class AGhostButton;
class AGhostGate;
class AGhostReplayController;

// Exactly two distinct E presses within the shared-clock window unlock one gate.
UCLASS(Blueprintable)
class GHOSTOFYOURSELF_API AGhostButtonGroup : public AActor
{
    GENERATED_BODY()
public:
    AGhostButtonGroup();
    virtual void Tick(float DeltaSeconds) override;
    bool ReceivePress(AGhostButton* Button, float Time, const FVector& ActorPosition);
    void ResetPuzzle();
    void GetButtons(TArray<AGhostButton*>& OutButtons) const;
    bool IsActiveRoom() const;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ghost|Buttons")
    TObjectPtr<AGhostReplayController> ControllerRef;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ghost|Buttons")
    TObjectPtr<AGhostGate> GateRef;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ghost|Buttons", meta = (ClampMin = "1"))
    int32 RoomNumber = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Buttons", meta = (ClampMin = "0.05", Units = "s"))
    float SyncWindowSeconds = 1.5f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ghost|Buttons")
    bool bUnlocked = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Buttons")
    bool bShowDebugStatus = true;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void ExpirePresses(float Time);
    TMap<TWeakObjectPtr<AGhostButton>, float> RecentPresses;
};
