#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostSecurityBeam.generated.h"

class AGhostReplayController;
class AGhostGate;
class UStaticMeshComponent;
class USceneComponent;
class ACharacter;

// A ghost shields the far side; the visible and dangerous beam stop at its virtual capsule.
UCLASS(Blueprintable)
class GHOSTOFYOURSELF_API AGhostSecurityBeam : public AActor
{
    GENERATED_BODY()
public:
    AGhostSecurityBeam();
    virtual void Tick(float DeltaSeconds) override;
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ghost|Beam")
    TObjectPtr<UStaticMeshComponent> EmitterMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ghost|Beam")
    TObjectPtr<UStaticMeshComponent> BeamMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ghost|Beam")
    TObjectPtr<UStaticMeshComponent> ImpactGlow;
    // Move this component to set the far end. Default beam follows local +X for 1200 cm.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ghost|Beam")
    TObjectPtr<USceneComponent> BeamEnd;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Ghost|Beam")
    TObjectPtr<AGhostReplayController> ControllerRef;
    // Optional exit gate, exclusively driven by this beam. Opens while the ghost blocks it.
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Ghost|Beam")
    TObjectPtr<AGhostGate> GateRef;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ghost|Beam", meta=(ClampMin="1"))
    int32 RoomNumber = 3;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ghost|Beam", meta=(ClampMin="0.1", Units="cm"))
    float BeamRadius = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ghost|Beam", meta=(ClampMin="0", Units="cm"))
    float ImpactGlowRadius = 6.f;
    // Ghosts have no physics capsule, so use a virtual upright capsule at their recorded origin.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ghost|Beam", meta=(ClampMin="1", Units="cm"))
    float GhostRadius = 34.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ghost|Beam", meta=(ClampMin="1", Units="cm"))
    float GhostHalfHeight = 88.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Ghost|Beam")
    bool bBlockedByGhost = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Ghost|Beam")
    bool bHazardActive = false;

    bool IntersectsCapsule(const FVector& Center, float Radius, float HalfHeight, float LengthFraction = 1.f) const;
    bool CrossedBeam(const FVector& Previous, const FVector& Current, float Radius, float HalfHeight, float LengthFraction = 1.f) const;
    bool FindGhostBlockDistance(const FVector& Center, float Radius, float HalfHeight, float& OutDistance) const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void RefreshMesh(float VisibleFraction = 1.f);
    FVector PreviousPlayerLocation = FVector::ZeroVector;
    bool bHavePreviousPlayerLocation = false;
    FDelegateHandle ResetHandle;
};
