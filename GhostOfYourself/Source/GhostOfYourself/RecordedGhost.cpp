#include "RecordedGhost.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogRecordedGhost, Log, All);

ARecordedGhost::ARecordedGhost()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    GhostMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GhostMesh"));
    GhostMesh->SetupAttachment(RootComponent);
    GhostMesh->SetMobility(EComponentMobility::Movable);
    GhostMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GhostMesh->SetGenerateOverlapEvents(false);
    GhostMesh->SetSimulatePhysics(false);
    GhostMesh->SetCastShadow(false);
    GhostMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.6f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        GhostMesh->SetStaticMesh(Cube.Object);
    }
    SetActorEnableCollision(false);

    GhostBody = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("GhostBody"));
    GhostBody->SetupAttachment(RootComponent);
    GhostBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GhostBody->SetGenerateOverlapEvents(false);
    GhostBody->SetCastShadow(false);
    GhostBody->SetComponentTickEnabled(false);
    GhostBody->SetVisibility(false);
}

FLinearColor ARecordedGhost::ColorForEcho(int32 Number)
{
    switch (FMath::Clamp(Number, 1, 3))
    {
    case 2: return FLinearColor(1.f, 0.75f, 0.015f, 1.f);
    case 3: return FLinearColor(0.025f, 1.f, 0.20f, 1.f);
    default: return FLinearColor(0.015f, 0.32f, 1.f, 1.f);
    }
}

void ARecordedGhost::SetEchoNumber(int32 Number)
{
    EchoNumber = FMath::Clamp(Number, 1, 3);
    if (IsValid(GhostMaterialInstance))
    {
        GhostMaterialInstance->SetVectorParameterValue(TEXT("EchoColor"), ColorForEcho(EchoNumber));
    }
}

void ARecordedGhost::ConfigureFromPlayer(const ACharacter* Player)
{
    const USkeletalMeshComponent* PlayerMesh = IsValid(Player) ? Player->GetMesh() : nullptr;
    if (!IsValid(PlayerMesh) || !PlayerMesh->GetSkinnedAsset())
    {
        return; // Preserve the visible cube fallback when no player body exists.
    }
    GhostBody->SetSkinnedAssetAndUpdate(PlayerMesh->GetSkinnedAsset());
    GhostBody->SetRelativeTransform(PlayerMesh->GetRelativeTransform());
    GhostBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GhostBody->SetComponentTickEnabled(false);
    GhostBody->SetVisibility(true);
    GhostMesh->SetVisibility(false);

    UMaterialInterface* Material = GhostMaterial;
    if (!Material)
    {
        Material = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/GhostOfYourself/Materials/M_GhostEcho.M_GhostEcho"), nullptr, LOAD_NoWarn);
    }
    if (Material)
    {
        GhostMaterialInstance = UMaterialInstanceDynamic::Create(Material, this);
        SetEchoNumber(EchoNumber);
    }
    else
    {
        UE_LOG(LogRecordedGhost, Warning, TEXT("Create M_GhostEcho with the supplied editor script for translucent colors. Using player materials for now."));
    }
    for (int32 Slot = 0; Slot < PlayerMesh->GetNumMaterials(); ++Slot)
    {
        GhostBody->SetMaterial(Slot, GhostMaterialInstance
            ? GhostMaterialInstance.Get() : PlayerMesh->GetMaterial(Slot));
    }
}

void ARecordedGhost::SetRecording(const TArray<FRecordedGhostSample>& InSamples)
{
    Samples = InSamples;
    PlaybackCursor = 0;
    EvaluateAtTime(0.f);
}

void ARecordedGhost::EvaluateAtTime(float Time)
{
    FTransform Transform;
    if (GhostRecording::Evaluate(Samples, Time, PlaybackCursor, Transform))
    {
        SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
        if (GhostBody->GetSkinnedAsset())
        {
            FTransform BodyTransform;
            if (GhostRecording::EvaluatePose(Samples, Time, PlaybackCursor, BodyTransform, EvaluatedBones)
                && EvaluatedBones.Num() == GhostBody->BoneSpaceTransforms.Num())
            {
                GhostBody->SetRelativeTransform(BodyTransform);
                GhostBody->BoneSpaceTransforms = EvaluatedBones;
                GhostBody->MarkRefreshTransformDirty();
                GhostBody->RefreshBoneTransforms();
            }
        }
    }
}
