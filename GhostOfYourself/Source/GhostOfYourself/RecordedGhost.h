#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostRecording.h"
#include "RecordedGhost.generated.h"

class UStaticMeshComponent;
class UPoseableMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ACharacter;

UCLASS(Blueprintable)
class GHOSTOFYOURSELF_API ARecordedGhost : public AActor
{
    GENERATED_BODY()

public:
    ARecordedGhost();
    void SetRecording(const TArray<FRecordedGhostSample>& InSamples);
    void EvaluateAtTime(float Time);
    void ConfigureFromPlayer(const ACharacter* Player);

    UFUNCTION(BlueprintCallable, Category = "Ghost|Appearance")
    void SetEchoNumber(int32 Number);

    static FLinearColor ColorForEcho(int32 Number);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
    TObjectPtr<UStaticMeshComponent> GhostMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Appearance")
    TObjectPtr<UPoseableMeshComponent> GhostBody;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Appearance")
    TObjectPtr<UMaterialInterface> GhostMaterial;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ghost|Appearance")
    int32 EchoNumber = 1;

private:
    UPROPERTY()
    TArray<FRecordedGhostSample> Samples;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> GhostMaterialInstance;

    TArray<FTransform> EvaluatedBones;

    int32 PlaybackCursor = 0;
};
