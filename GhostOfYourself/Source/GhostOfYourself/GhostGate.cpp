#include "GhostGate.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AGhostGate::AGhostGate()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    GateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateMesh"));
    GateMesh->SetupAttachment(RootComponent);
    GateMesh->SetMobility(EComponentMobility::Movable);
    GateMesh->SetRelativeLocation(FVector(0.f, 0.f, 125.f));
    GateMesh->SetRelativeScale3D(FVector(2.f, 0.2f, 2.5f));
    GateMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    GateMesh->SetGenerateOverlapEvents(false);
    GateMesh->SetSimulatePhysics(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        GateMesh->SetStaticMesh(Cube.Object);
    }

    LockBarrier = CreateDefaultSubobject<UBoxComponent>(TEXT("LockBarrier"));
    LockBarrier->SetupAttachment(RootComponent);
    LockBarrier->SetMobility(EComponentMobility::Movable);
    LockBarrier->SetBoxExtent(FVector(50.f));
    LockBarrier->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    LockBarrier->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LockBarrier->SetGenerateOverlapEvents(false);
    LockBarrier->SetHiddenInGame(true);
}

void AGhostGate::BeginPlay()
{
    Super::BeginPlay();
    ClosedMeshLocation = GateMesh->GetRelativeLocation();
    bHasClosedMeshLocation = true;
    SetOpen(false);
}

void AGhostGate::SetOpen(bool bOpen)
{
    bIsOpen = bOpen && !bLockedClosed;
    // Keep a physical door: its collision moves upward with its visible mesh.
    if (GateMesh->GetCollisionEnabled() != ECollisionEnabled::QueryAndPhysics)
    {
        GateMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    if (!GateMesh->IsVisible())
    {
        GateMesh->SetVisibility(true);
    }
}

void AGhostGate::LockClosed()
{
    if (!bHasClosedMeshLocation)
    {
        ClosedMeshLocation = GateMesh->GetRelativeLocation();
        bHasClosedMeshLocation = true;
    }
    FTransform BarrierTransform = GateMesh->GetRelativeTransform();
    BarrierTransform.SetLocation(ClosedMeshLocation);
    LockBarrier->SetRelativeTransform(BarrierTransform);
    if (GateMesh->GetStaticMesh())
    {
        LockBarrier->SetBoxExtent(GateMesh->GetStaticMesh()->GetBounds().BoxExtent);
    }
    LockBarrier->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    bLockedClosed = true;
    SetOpen(false);
}

bool AGhostGate::IsPlayerBeyondGate(const ACharacter* Player, const FVector& Destination) const
{
    if (!IsValid(Player))
    {
        return false;
    }
    return IsPositionBeyondGate(Player->GetActorLocation(), Destination,
        Player->GetCapsuleComponent()->GetScaledCapsuleRadius());
}

bool AGhostGate::IsPositionBeyondGate(const FVector& Position, const FVector& Destination, float Radius) const
{
    // The native door is thin along its local Y axis. Actor yaw may orient it.
    const FVector Closed = bHasClosedMeshLocation ? ClosedMeshLocation : GateMesh->GetRelativeLocation();
    const FVector PlaneOrigin = GetActorTransform().TransformPosition(Closed);
    const FVector Normal = GateMesh->GetComponentTransform().TransformVectorNoScale(FVector::YAxisVector).GetSafeNormal();
    const float DestinationSide = FVector::DotProduct(Destination - PlaneOrigin, Normal);
    if (FMath::IsNearlyZero(DestinationSide))
    {
        return false; // Exit must be beyond the doorway, not centered on its plane.
    }
    const float HalfThickness = GateMesh->GetStaticMesh()
        ? GateMesh->GetStaticMesh()->GetBounds().BoxExtent.Y * FMath::Abs(GateMesh->GetComponentScale().Y) : 10.f;
    const float Clearance = HalfThickness + FMath::Max(0.f, Radius) + 2.f;
    const float PlayerSide = FVector::DotProduct(Position - PlaneOrigin, Normal)
        * FMath::Sign(DestinationSide);
    return PlayerSide >= Clearance;
}

void AGhostGate::AdvanceMovement(float DeltaSeconds)
{
    if (!bHasClosedMeshLocation)
    {
        ClosedMeshLocation = GateMesh->GetRelativeLocation();
        bHasClosedMeshLocation = true;
    }
    const FVector Target = ClosedMeshLocation
        + (bIsOpen ? FVector(0.f, 0.f, FMath::Max(0.f, OpenHeight)) : FVector::ZeroVector);
    const FVector Next = FMath::VInterpConstantTo(GateMesh->GetRelativeLocation(),
        Target, FMath::Max(0.f, DeltaSeconds), FMath::Max(1.f, TravelSpeed));
    if (!Next.Equals(GateMesh->GetRelativeLocation(), UE_SMALL_NUMBER))
    {
        GateMesh->SetRelativeLocation(Next);
    }
}
