#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostPressurePlateGroup.generated.h"

class AGhostReplayController;
class AGhostGate;
class AGhostPressurePlate;

// All linked plates must stay occupied. An empty group never opens its gate.
UCLASS(Blueprintable)
class GHOSTOFYOURSELF_API AGhostPressurePlateGroup : public AActor
{
    GENERATED_BODY()
public:
    AGhostPressurePlateGroup();
    virtual void Tick(float DeltaSeconds) override;
    void GetPlates(TArray<AGhostPressurePlate*>& OutPlates) const;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Ghost|Plates")
    TObjectPtr<AGhostReplayController> ControllerRef;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Ghost|Plates")
    TObjectPtr<AGhostGate> GateRef;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Ghost|Plates", meta=(ClampMin="1"))
    int32 RoomNumber = 1;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Ghost|Plates")
    bool bAllOccupied = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Ghost|Plates")
    int32 OccupiedPlateCount = 0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Ghost|Plates")
    int32 TotalPlateCount = 0;

protected:
    virtual void BeginPlay() override;
};
