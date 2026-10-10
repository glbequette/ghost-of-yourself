#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostPressurePlate.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class AGhostGate;
class AGhostReplayController;

// Checks actor origins, so collision-disabled recorded ghosts can hold a plate.
UCLASS(Blueprintable)
class GHOSTOFYOURSELF_API AGhostPressurePlate : public AActor
{
    GENERATED_BODY()

public:
    AGhostPressurePlate();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category = "Ghost|Plate")
    bool IsActorOnPlate(const AActor* ActorToCheck) const;

    // Tick supplies the local player and the linked controller's current ghost.
    void UpdateOccupancy(const AActor* Player, const AActor* Ghost);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Plate")
    TObjectPtr<UStaticMeshComponent> PlateMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Plate")
    TObjectPtr<UBoxComponent> ActivationBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Plate")
    TObjectPtr<UPointLightComponent> ActiveLight;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ghost|Plate")
    TObjectPtr<AGhostGate> GateRef;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ghost|Plate")
    TObjectPtr<AGhostReplayController> ControllerRef;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ghost|Plate")
    bool bOccupied = false;

protected:
    virtual void BeginPlay() override;
};
