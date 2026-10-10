#include "GhostRecording.h"

bool GhostRecording::Evaluate(const TArray<FRecordedGhostSample>& Samples,
    float Time, int32& Cursor, FTransform& OutTransform)
{
    if (Samples.IsEmpty())
    {
        Cursor = 0;
        return false;
    }
    if (Samples.Num() == 1 || Time <= Samples[0].Time)
    {
        Cursor = 0;
        OutTransform = Samples[0].Transform;
        return true;
    }
    if (Time >= Samples.Last().Time)
    {
        Cursor = FMath::Max(0, Samples.Num() - 2);
        OutTransform = Samples.Last().Transform;
        return true;
    }

    Cursor = FMath::Clamp(Cursor, 0, Samples.Num() - 2);
    if (Time < Samples[Cursor].Time)
    {
        Cursor = 0;
    }
    while (Cursor < Samples.Num() - 2 && Time >= Samples[Cursor + 1].Time)
    {
        ++Cursor;
    }
    const FRecordedGhostSample& A = Samples[Cursor];
    const FRecordedGhostSample& B = Samples[Cursor + 1];
    const float Duration = B.Time - A.Time;
    const float Alpha = Duration > UE_SMALL_NUMBER
        ? FMath::Clamp((Time - A.Time) / Duration, 0.f, 1.f) : 1.f;

    OutTransform = FTransform(
        FQuat::Slerp(A.Transform.GetRotation(), B.Transform.GetRotation(), Alpha).GetNormalized(),
        FMath::Lerp(A.Transform.GetLocation(), B.Transform.GetLocation(), Alpha),
        FMath::Lerp(A.Transform.GetScale3D(), B.Transform.GetScale3D(), Alpha));
    return true;
}

bool GhostRecording::EvaluatePose(const TArray<FRecordedGhostSample>& Samples,
    float Time, int32& Cursor, FTransform& OutBodyTransform, TArray<FTransform>& OutBones)
{
    FTransform ActorTransform;
    if (!Evaluate(Samples, Time, Cursor, ActorTransform))
    {
        OutBones.Reset();
        return false;
    }
    const FRecordedGhostSample* Held = nullptr;
    if (Samples.Num() == 1 || Time <= Samples[0].Time)
    {
        Held = &Samples[0];
    }
    else if (Time >= Samples.Last().Time)
    {
        Held = &Samples.Last();
    }
    if (Held)
    {
        OutBodyTransform = Held->BodyTransform;
        OutBones = Held->BoneTransforms;
        return !OutBones.IsEmpty();
    }
    const FRecordedGhostSample& A = Samples[Cursor];
    const FRecordedGhostSample& B = Samples[Cursor + 1];
    if (A.BoneTransforms.IsEmpty() || A.BoneTransforms.Num() != B.BoneTransforms.Num())
    {
        OutBones.Reset();
        return false;
    }
    const float Duration = B.Time - A.Time;
    const float Alpha = Duration > UE_SMALL_NUMBER
        ? FMath::Clamp((Time - A.Time) / Duration, 0.f, 1.f) : 1.f;
    OutBodyTransform.Blend(A.BodyTransform, B.BodyTransform, Alpha);
    OutBones.SetNumUninitialized(A.BoneTransforms.Num());
    for (int32 Bone = 0; Bone < OutBones.Num(); ++Bone)
    {
        OutBones[Bone].Blend(A.BoneTransforms[Bone], B.BoneTransforms[Bone], Alpha);
    }
    return true;
}
