#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostGate.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class ACharacter;
class UTextRenderComponent;

// One pressure plate OR button group owns this gate. Its origin is at floor level.
UCLASS(Blueprintable)
class GHOSTOFYOURSELF_API AGhostGate : public AActor
{
    GENERATED_BODY()

public:
    AGhostGate();

    UFUNCTION(BlueprintCallable, Category = "Ghost|Gate")
    void SetOpen(bool bOpen);

    UFUNCTION(BlueprintCallable, Category = "Ghost|Door Number")
    void SetDestinationRoomNumber(int32 Number);

    virtual void OnConstruction(const FTransform& Transform) override;

    // Zero leaves internal/unnumbered doors blank. This names the room beyond the door.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Door Number", meta = (ClampMin = "0"))
    int32 DestinationRoomNumber = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Door Number", meta = (ClampMin = "5", ClampMax = "100", Units = "cm"))
    float RoomNumberSize = 26.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Door Number")
    FColor RoomNumberColor = FColor(18, 30, 42);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Door Number")
    TObjectPtr<UTextRenderComponent> FrontRoomNumber;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Door Number")
    TObjectPtr<UTextRenderComponent> BackRoomNumber;

    // The linked plate or button group advances motion after updating its state each frame.
    void AdvanceMovement(float DeltaSeconds);

    void LockClosed();
    void ResetForRoomCheat();
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

public:
    // A closing door may still physically clear a player already walking out.
    bool AllowsExitForPlayer(const ACharacter* Player) const;

private:
    void RefreshRoomNumber();
    FVector ClosedMeshLocation = FVector::ZeroVector;
    bool bHasClosedMeshLocation = false;
};
