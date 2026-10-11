#include "GhostSecurityBeam.h"
#include "GhostReplayController.h"
#include "GhostGate.h"
#include "RecordedGhost.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AGhostSecurityBeam::AGhostSecurityBeam()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    BeamEnd = CreateDefaultSubobject<USceneComponent>(TEXT("BeamEnd"));
    BeamEnd->SetupAttachment(RootComponent);
    BeamEnd->SetRelativeLocation(FVector(1200.f, 0.f, 0.f));
    EmitterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EmitterMesh"));
    EmitterMesh->SetupAttachment(RootComponent);
    EmitterMesh->SetRelativeScale3D(FVector(.15f, .25f, .25f));
    BeamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeamMesh"));
    BeamMesh->SetupAttachment(RootComponent);
    ImpactGlow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ImpactGlow"));
    ImpactGlow->SetupAttachment(RootComponent);
    ImpactGlow->SetVisibility(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Cube.Succeeded()) { EmitterMesh->SetStaticMesh(Cube.Object); }
    if (Cylinder.Succeeded()) { BeamMesh->SetStaticMesh(Cylinder.Object); }
    if (Sphere.Succeeded()) { ImpactGlow->SetStaticMesh(Sphere.Object); }
    for (UStaticMeshComponent* Mesh : { EmitterMesh.Get(), BeamMesh.Get(), ImpactGlow.Get() })
    {
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetSimulatePhysics(false);
        Mesh->SetCastShadow(false);
    }
    RefreshMesh();
}

void AGhostSecurityBeam::RefreshMesh(float VisibleFraction)
{
    const FVector End = BeamEnd->GetRelativeLocation() * FMath::Clamp(VisibleFraction, 0.f, 1.f);
    BeamMesh->SetRelativeLocation(End * .5f);
    BeamMesh->SetRelativeRotation(FRotationMatrix::MakeFromZ(End.GetSafeNormal()).Rotator());
    // Engine cylinder is 100 cm tall and 100 cm in diameter.
    BeamMesh->SetRelativeScale3D(FVector(BeamRadius / 50.f, BeamRadius / 50.f, End.Size() / 100.f));
}

bool AGhostSecurityBeam::FindGhostBlockDistance(const FVector& Center, float Radius, float HalfHeight, float& OutDistance) const
{
    const FVector Start = GetActorLocation();
    const FVector End = BeamEnd->GetComponentLocation();
    OutDistance = FVector::Distance(Start, End);
    if (Start.Equals(End)) { return false; }
    const FVector Axis(0.f, 0.f, FMath::Max(0.f, HalfHeight - Radius));
    const FVector CapsuleStart = Center - Axis;
    const FVector CapsuleEnd = Center + Axis;
    FVector OnBeam, OnCapsule;
    FMath::SegmentDistToSegmentSafe(Start, End, CapsuleStart, CapsuleEnd, OnBeam, OnCapsule);
    const float RadiusSquared = FMath::Square(FMath::Max(0.f, Radius) + BeamRadius);
    if (FVector::DistSquared(OnBeam, OnCapsule) > RadiusSquared) { return false; }
    const auto Inside = [&](const FVector& Point)
    {
        return FVector::DistSquared(Point, FMath::ClosestPointOnSegment(Point, CapsuleStart, CapsuleEnd)) <= RadiusSquared;
    };
    if (Inside(Start)) { OutDistance = 0.f; return true; }
    // Distance to a convex capsule decreases to the closest point along this segment.
    // Find its first surface contact, rather than stopping at the ghost's center.
    double Near = 0.;
    double Far = FMath::Clamp(FVector::DotProduct(OnBeam - Start, End - Start) / (End - Start).SizeSquared(), 0., 1.);
    for (int32 Iteration = 0; Iteration < 24; ++Iteration)
    {
        const double Mid = (Near + Far) * .5;
        if (Inside(FMath::Lerp(Start, End, Mid))) { Far = Mid; }
        else { Near = Mid; }
    }
    OutDistance *= Far;
    return true;
}

void AGhostSecurityBeam::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshMesh();
}

void AGhostSecurityBeam::BeginPlay()
{
    Super::BeginPlay();
    if (IsValid(ControllerRef))
    {
        AddTickPrerequisiteActor(ControllerRef);
        ResetHandle = ControllerRef->OnAttemptReset.AddLambda([this] { bHavePreviousPlayerLocation = false; });
    }
    else { UE_LOG(LogTemp, Warning, TEXT("%s: assign Controller Ref; security beam is disabled."), *GetName()); }
}

void AGhostSecurityBeam::EndPlay(const EEndPlayReason::Type Reason)
{
    if (IsValid(ControllerRef)) { ControllerRef->OnAttemptReset.Remove(ResetHandle); }
    Super::EndPlay(Reason);
}

bool AGhostSecurityBeam::IntersectsCapsule(const FVector& Center, float Radius, float HalfHeight, float LengthFraction) const
{
    const FVector Start = GetActorLocation();
    const FVector End = FMath::Lerp(Start, BeamEnd->GetComponentLocation(), FMath::Clamp(LengthFraction, 0.f, 1.f));
    if (Start.Equals(End)) { return false; }
    const FVector Axis(0.f, 0.f, FMath::Max(0.f, HalfHeight - Radius));
    FVector OnBeam, OnCapsule;
    FMath::SegmentDistToSegmentSafe(Start, End, Center - Axis, Center + Axis, OnBeam, OnCapsule);
    const float TotalRadius = FMath::Max(0.f, Radius) + BeamRadius;
    return FVector::DistSquared(OnBeam, OnCapsule) <= FMath::Square(TotalRadius);
}

bool AGhostSecurityBeam::CrossedBeam(const FVector& Previous, const FVector& Current, float Radius, float HalfHeight, float LengthFraction) const
{
    // Sweep against a capsule-expanded beam box to catch crossings between frames.
    // The conservative bounds also support beams rotated or tilted in the editor.
    const FVector Start = GetActorLocation();
    const FVector End = FMath::Lerp(Start, BeamEnd->GetComponentLocation(), FMath::Clamp(LengthFraction, 0.f, 1.f));
    if (Start.Equals(End)) { return false; }
    const FQuat Rotation = FRotationMatrix::MakeFromX((End - Start).GetSafeNormal()).ToQuat();
    const FVector A = Rotation.UnrotateVector(Previous - (Start + End) * .5f);
    const FVector B = Rotation.UnrotateVector(Current - (Start + End) * .5f);
    const FVector Axis = Rotation.UnrotateVector(FVector::UpVector).GetAbs() * FMath::Max(0.f, HalfHeight - Radius);
    const FVector Extent = FVector(FVector::Distance(Start, End) * .5f, 0.f, 0.f)
        + Axis + FVector(Radius + BeamRadius);
    const FVector Delta = B - A;
    double Enter = 0., Leave = 1.;
    for (int32 Index = 0; Index < 3; ++Index)
    {
        if (FMath::IsNearlyZero(Delta[Index]))
        {
            if (FMath::Abs(A[Index]) > Extent[Index]) { return false; }
            continue;
        }
        double Near = (-Extent[Index] - A[Index]) / Delta[Index];
        double Far = (Extent[Index] - A[Index]) / Delta[Index];
        if (Near > Far) { Swap(Near, Far); }
        Enter = FMath::Max(Enter, Near);
        Leave = FMath::Min(Leave, Far);
        if (Enter > Leave) { return false; }
    }
    return true;
}

void AGhostSecurityBeam::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const bool bInRoom = IsValid(ControllerRef) && ControllerRef->CurrentRoomNumber == RoomNumber;
    const float FullLength = FVector::Distance(GetActorLocation(), BeamEnd->GetComponentLocation());
    float BlockDistance = FullLength;
    bBlockedByGhost = false;
    if (bInRoom)
    {
        TArray<ARecordedGhost*> Ghosts;
        ControllerRef->GetGhostActors(Ghosts);
        for (ARecordedGhost* Ghost : Ghosts)
        {
            float Distance = 0.f;
            if (FindGhostBlockDistance(Ghost->GetActorLocation(), GhostRadius, GhostHalfHeight, Distance))
            {
                bBlockedByGhost = true;
                BlockDistance = FMath::Min(BlockDistance, Distance);
            }
        }
    }
    const float VisibleFraction = bBlockedByGhost && FullLength > UE_SMALL_NUMBER ? BlockDistance / FullLength : 1.f;
    // Only the section after the ghost is safe. Recording and reset fades remain harmless.
    bHazardActive = bInRoom && FullLength * VisibleFraction > UE_SMALL_NUMBER
        && !ControllerRef->bRecording && !ControllerRef->bResetInProgress;
    RefreshMesh(VisibleFraction);
    BeamMesh->SetVisibility(bInRoom && FullLength > UE_SMALL_NUMBER && VisibleFraction > UE_SMALL_NUMBER);
    ImpactGlow->SetVisibility(bBlockedByGhost && ImpactGlowRadius > 0.f);
    if (bBlockedByGhost)
    {
        ImpactGlow->SetRelativeLocation(BeamEnd->GetRelativeLocation() * VisibleFraction);
        const float Pulse = 1.f + .15f * FMath::Sin(ControllerRef->Clock * 8.f);
        ImpactGlow->SetRelativeScale3D(FVector(ImpactGlowRadius * Pulse / 50.f));
        // Reuse the assigned emissive material so existing placed beams need no new material hookup.
        if (ImpactGlow->GetMaterial(0) != BeamMesh->GetMaterial(0))
        {
            ImpactGlow->SetMaterial(0, BeamMesh->GetMaterial(0));
        }
    }
    if (IsValid(GateRef))
    {
        if (bInRoom) { GateRef->SetOpen(bBlockedByGhost && !ControllerRef->bRecording && !ControllerRef->bResetInProgress); }
        // Keep closing after the exit zone locks this gate and advances to the next room.
        GateRef->AdvanceMovement(DeltaSeconds);
    }
    ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (!bHazardActive || !IsValid(Player)) { bHavePreviousPlayerLocation = false; return; }
    const FVector Position = Player->GetActorLocation();
    const UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
    const float Radius = Capsule->GetScaledCapsuleRadius();
    const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    const bool bHit = IntersectsCapsule(Position, Radius, HalfHeight, VisibleFraction)
        || (bHavePreviousPlayerLocation && CrossedBeam(PreviousPlayerLocation, Position, Radius, HalfHeight, VisibleFraction));
    PreviousPlayerLocation = Position;
    bHavePreviousPlayerLocation = true;
    if (bHit)
    {
        bHavePreviousPlayerLocation = false;
        ControllerRef->CancelOrRestart();
    }
}
