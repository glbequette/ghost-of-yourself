#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostExitZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class AGhostReplayController;
class AGhostGate;
class ATargetPoint;
class ACharacter;

// One-way transition between rooms in the same map; no level travel or pause.
UCLASS(Blueprintable)
class GHOSTOFYOURSELF_API AGhostExitZone : public AActor
{
    GENERATED_BODY()

public:
    AGhostExitZone();
    virtual void Tick(float DeltaSeconds) override;

    // Only the current local first-person character can complete this zone.
    bool TryCompleteForActor(const AActor* ActorToCheck);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Exit")
    TObjectPtr<UBoxComponent> ExitBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Exit")
    TObjectPtr<UStaticMeshComponent> ExitMarker;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Exit")
    TObjectPtr<UTextRenderComponent> ExitLabel;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ghost|Exit")
    TObjectPtr<AGhostReplayController> ControllerRef;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ghost|Exit")
    TObjectPtr<AGhostGate> GateRef;

    // Optional floor-level TargetPoint in the next room; otherwise use entry position.
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ghost|Exit")
    TObjectPtr<ATargetPoint> NextRoomStart;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Exit", meta = (ClampMin = "1"))
    int32 FromRoomNumber = 1;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ghost|Exit")
    bool bCompleted = false;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    bool TryCompleteForActorSwept(const AActor* ActorToCheck, const FVector* PreviousPosition);
    void ClearPreviousPosition();
    void WriteExitDiagnostic(const TCHAR* Reason, const ACharacter* Player);
    FString LastDiagnosticReason;
    TWeakObjectPtr<ACharacter> PreviousPlayer;
    FVector PreviousPlayerPosition = FVector::ZeroVector;
    bool bHasPreviousPosition = false;
};
