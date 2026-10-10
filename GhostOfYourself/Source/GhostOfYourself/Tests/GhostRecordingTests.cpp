#include "../GhostRecording.h"
#include "../RecordedGhost.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostRecordingPlaybackTest, "GhostOfYourself.Recording.Playback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGhostRecordingPlaybackTest::RunTest(const FString& Parameters)
{
    TArray<FRecordedGhostSample> Samples;
    int32 Cursor = 0;
    FTransform Result;
    TestFalse(TEXT("Empty recording is safe"), GhostRecording::Evaluate(Samples, 0.f, Cursor, Result));

    for (int32 Index = 0; Index < 4; ++Index)
    {
        FRecordedGhostSample Sample;
        Sample.Time = static_cast<float>(Index);
        Sample.Transform = FTransform(FRotator(0.f, Index * 30.f, 0.f), FVector(Index * 100.f, 0.f, 0.f));
        Samples.Add(Sample);
    }
    TestTrue(TEXT("Can evaluate"), GhostRecording::Evaluate(Samples, 0.5f, Cursor, Result));
    TestTrue(TEXT("Midpoint interpolates instead of teleporting"),
        Result.GetLocation().Equals(FVector(50.f, 0.f, 0.f), 0.01f));
    TestTrue(TEXT("Facing interpolates"), FMath::IsNearlyEqual(Result.Rotator().Yaw, 15.f, 0.01f));

    GhostRecording::Evaluate(Samples, 2.5f, Cursor, Result);
    TestTrue(TEXT("Slow frame crosses multiple samples"),
        Result.GetLocation().Equals(FVector(250.f, 0.f, 0.f), 0.01f));
    GhostRecording::Evaluate(Samples, 100.f, Cursor, Result);
    TestTrue(TEXT("Holds endpoint beyond duration"), Result.Equals(Samples.Last().Transform));
    GhostRecording::Evaluate(Samples, 0.f, Cursor, Result);
    TestTrue(TEXT("Restart returns to start"), Result.Equals(Samples[0].Transform));
    GhostRecording::Evaluate(Samples, 0.25f, Cursor, Result);
    TestTrue(TEXT("Interpolation resumes after restart"),
        Result.GetLocation().Equals(FVector(25.f, 0.f, 0.f), 0.01f));
    GhostRecording::Evaluate(Samples, -1.f, Cursor, Result);
    TestTrue(TEXT("Negative time clamps to start"), Result.Equals(Samples[0].Transform));

    // Real recording timestamps are uneven at varying frame rates.
    Samples[1].Time = 0.07f;
    Samples[2].Time = 0.19f;
    Samples[3].Time = 0.31f;
    Cursor = 999;
    GhostRecording::Evaluate(Samples, 0.13f, Cursor, Result);
    TestTrue(TEXT("Uneven timestamps interpolate using their actual interval"),
        Result.GetLocation().Equals(FVector(150.f, 0.f, 0.f), 0.01f));
    GhostRecording::Evaluate(Samples, 0.25f, Cursor, Result);
    GhostRecording::Evaluate(Samples, 0.035f, Cursor, Result);
    TestTrue(TEXT("Rewinding into an earlier interval resets the cursor"),
        Result.GetLocation().Equals(FVector(50.f, 0.f, 0.f), 0.01f));

    Samples[0].Transform = FTransform(FRotator(0.f, 170.f, 0.f), FVector::ZeroVector, FVector(1.f));
    Samples[1].Transform = FTransform(FRotator(0.f, -170.f, 0.f), FVector(100.f, 0.f, 0.f), FVector(3.f));
    GhostRecording::Evaluate(Samples, 0.035f, Cursor, Result);
    TestTrue(TEXT("Facing takes the short path across the yaw boundary"),
        FMath::IsNearlyEqual(FMath::Abs(Result.Rotator().Yaw), 180.f, 0.01f));
    TestTrue(TEXT("Scale interpolates"), Result.GetScale3D().Equals(FVector(2.f), 0.01f));

    Samples.SetNum(1);
    TestTrue(TEXT("Single sample is safe"), GhostRecording::Evaluate(Samples, 20.f, Cursor, Result));
    TestTrue(TEXT("Single sample holds"), Result.Equals(Samples[0].Transform));

    FRecordedGhostSample Duplicate;
    Duplicate.Time = 0.f;
    Duplicate.Transform.SetLocation(FVector(5.f, 0.f, 0.f));
    Samples.Add(Duplicate);
    GhostRecording::Evaluate(Samples, 1.f, Cursor, Result);
    TestTrue(TEXT("Zero-duration recording holds endpoint"), Result.Equals(Duplicate.Transform));
    Samples.SetNum(2);
    Samples[0].Time = 0.f;
    Samples[1].Time = 1.f;
    Samples[0].BodyTransform = FTransform(FVector(0.f, 0.f, -90.f));
    Samples[1].BodyTransform = FTransform(FVector(0.f, 0.f, -70.f));
    Samples[0].BoneTransforms = { FTransform(FRotator(0.f, 170.f, 0.f), FVector::ZeroVector) };
    Samples[1].BoneTransforms = { FTransform(FRotator(0.f, -170.f, 0.f), FVector(10.f, 0.f, 0.f)) };
    TArray<FTransform> Bones;
    TestTrue(TEXT("Recorded pose evaluates"), GhostRecording::EvaluatePose(Samples, .5f, Cursor, Result, Bones));
    TestTrue(TEXT("Body offset interpolates"), Result.GetLocation().Equals(FVector(0.f, 0.f, -80.f)));
    TestEqual(TEXT("Pose keeps bone count"), Bones.Num(), 1);
    if (Bones.Num() == 1)
    {
        TestTrue(TEXT("Bone translation interpolates"), Bones[0].GetLocation().Equals(FVector(5.f, 0.f, 0.f)));
        TestTrue(TEXT("Bone rotation takes shortest path"), FMath::IsNearlyEqual(FMath::Abs(Bones[0].Rotator().Yaw), 180.f, .01f));
    }
    GhostRecording::EvaluatePose(Samples, 100.f, Cursor, Result, Bones);
    TestTrue(TEXT("Pose holds endpoint"), Bones.Num() == 1 && Bones[0].Equals(Samples.Last().BoneTransforms[0]));
    GhostRecording::EvaluatePose(Samples, 0.f, Cursor, Result, Bones);
    TestTrue(TEXT("Pose restarts"), Bones.Num() == 1 && Bones[0].Equals(Samples[0].BoneTransforms[0]));
    Samples[1].BoneTransforms.Reset();
    TestFalse(TEXT("Incompatible poses are rejected safely"), GhostRecording::EvaluatePose(Samples, .5f, Cursor, Result, Bones));
    TestEqual(TEXT("Invalid pose clears old bones"), Bones.Num(), 0);
    Samples.Reset();
    TestFalse(TEXT("Empty pose is safe"), GhostRecording::EvaluatePose(Samples, 0.f, Cursor, Result, Bones));
    const FLinearColor Blue = ARecordedGhost::ColorForEcho(1);
    const FLinearColor Yellow = ARecordedGhost::ColorForEcho(2);
    const FLinearColor Green = ARecordedGhost::ColorForEcho(3);
    TestTrue(TEXT("First echo is blue"), Blue.B > Blue.G && Blue.G > Blue.R);
    TestTrue(TEXT("Second echo is yellow"), Yellow.R > Yellow.B && Yellow.G > Yellow.B);
    TestTrue(TEXT("Third echo is green"), Green.G > Green.R && Green.G > Green.B);
    return true;
}
#endif
