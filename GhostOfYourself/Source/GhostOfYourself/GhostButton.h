#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostButton.generated.h"

class AGhostButtonGroup;
class UStaticMeshComponent;
class UPointLightComponent;

UCLASS(Blueprintable)
class GHOSTOFYOURSELF_API AGhostButton : public AActor
{
    GENERATED_BODY()
public:
    AGhostButton();
    bool IsInRange(const FVector& ActorPosition) const;
    void SetFeedback(bool bRecentPress, bool bUnlocked);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Button")
    TObjectPtr<UStaticMeshComponent> ButtonMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Button")
    TObjectPtr<UPointLightComponent> IndicatorLight;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ghost|Button")
    TObjectPtr<AGhostButtonGroup> GroupRef;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Button", meta = (ClampMin = "25", Units = "cm"))
    float InteractionDistance = 180.f;
};
