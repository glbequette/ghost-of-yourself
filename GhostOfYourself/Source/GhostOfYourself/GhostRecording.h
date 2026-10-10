#pragma once

#include "CoreMinimal.h"
#include "GhostRecording.generated.h"

class AGhostButton;

USTRUCT()
struct FRecordedGhostButtonPress
{
    GENERATED_BODY()

    UPROPERTY()
    float Time = 0.f;

    // An actor reference identifies the exact panel, rather than its current nearest neighbor.
    UPROPERTY()
    TWeakObjectPtr<AGhostButton> Button;
};

// Separate from the existing Blueprint ST_GhostSample asset, which is preserved.
USTRUCT(BlueprintType)
struct FRecordedGhostSample
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
    float Time = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
    FTransform Transform = FTransform::Identity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
    FTransform BodyTransform = FTransform::Identity;

    // Local to each bone's parent; empty for transform-only samples.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
    TArray<FTransform> BoneTransforms;
};

namespace GhostRecording
{
    // Cursor avoids scanning the entire recording each frame; rewinding is supported.
    GHOSTOFYOURSELF_API bool Evaluate(const TArray<FRecordedGhostSample>& Samples,
        float Time, int32& Cursor, FTransform& OutTransform);

    GHOSTOFYOURSELF_API bool EvaluatePose(const TArray<FRecordedGhostSample>& Samples,
        float Time, int32& Cursor, FTransform& OutBodyTransform, TArray<FTransform>& OutBones);
}
