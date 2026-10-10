#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostGate.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class ACharacter;

// One pressure plate OR button group owns this gate. Its origin is at floor level.
UCLASS(Blueprintable)
class GHOSTOFYOURSELF_API AGhostGate : public AActor
{
    GENERATED_BODY()

public:
    AGhostGate();

    UFUNCTION(BlueprintCallable, Category = "Ghost|Gate")
    void SetOpen(bool bOpen);

    // The linked plate or button group advances motion after updating its state each frame.
    void AdvanceMovement(float DeltaSeconds);

    void LockClosed();
    bool IsPlayerBeyondGate(const ACharacter* Player, const FVector& Destination) const;
    bool IsPositionBeyondGate(const FVector& Position, const FVector& Destination, float Radius) const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Gate")
    TObjectPtr<UStaticMeshComponent> GateMesh;

    // Blocks return immediately while the visible door finishes lowering.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Gate")
    TObjectPtr<UBoxComponent> LockBarrier;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Gate", meta = (ClampMin = "0", Units = "cm"))
    float OpenHeight = 270.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Gate", meta = (ClampMin = "1", Units = "cm/s"))
    float TravelSpeed = 400.f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ghost|Gate")
    bool bIsOpen = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ghost|Gate")
    bool bLockedClosed = false;

protected:
    virtual void BeginPlay() override;

private:
    FVector ClosedMeshLocation = FVector::ZeroVector;
    bool bHasClosedMeshLocation = false;
};
