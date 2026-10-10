#include "GhostPressurePlate.h"
#include "GhostGate.h"
#include "GhostReplayController.h"
#include "RecordedGhost.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogGhostPlate, Log, All);

AGhostPressurePlate::AGhostPressurePlate()
{
    PrimaryActorTick.bCanEverTick = true;
    // Recording, playback, and resets happen in the controller's PostPhysics tick.
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    PlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlateMesh"));
    PlateMesh->SetupAttachment(RootComponent);
    PlateMesh->SetMobility(EComponentMobility::Movable);
    PlateMesh->SetRelativeLocation(FVector(0.f, 0.f, 5.f));
    PlateMesh->SetRelativeScale3D(FVector(2.f, 2.f, 0.1f));
    PlateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PlateMesh->SetGenerateOverlapEvents(false);
    PlateMesh->SetSimulatePhysics(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        PlateMesh->SetStaticMesh(Cube.Object);
    }

    ActivationBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ActivationBox"));
    ActivationBox->SetupAttachment(RootComponent);
    ActivationBox->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
    ActivationBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));
    ActivationBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ActivationBox->SetGenerateOverlapEvents(false);
    ActivationBox->SetHiddenInGame(true);

    ActiveLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ActiveLight"));
    ActiveLight->SetupAttachment(RootComponent);
    ActiveLight->SetMobility(EComponentMobility::Movable);
    ActiveLight->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
    ActiveLight->SetLightColor(FLinearColor(0.f, 1.f, 1.f));
    ActiveLight->SetIntensity(300.f);
    ActiveLight->SetAttenuationRadius(220.f);
    ActiveLight->SetCastShadows(false);
    ActiveLight->SetVisibility(false);
}

void AGhostPressurePlate::BeginPlay()
{
    Super::BeginPlay();
    if (IsValid(ControllerRef))
    {
        AddTickPrerequisiteActor(ControllerRef);
    }
    else
    {
        UE_LOG(LogGhostPlate, Warning, TEXT("%s: assign Controller Ref to detect the saved ghost."), *GetName());
    }
    if (!IsValid(GateRef))
    {
        UE_LOG(LogGhostPlate, Warning, TEXT("%s: assign Gate Ref to open a gate."), *GetName());
    }
}

void AGhostPressurePlate::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const AActor* Ghost = IsValid(ControllerRef) ? ControllerRef->GhostRef.Get() : nullptr;
    UpdateOccupancy(UGameplayStatics::GetPlayerCharacter(this, 0), Ghost);
    if (IsValid(GateRef))
    {
        GateRef->AdvanceMovement(DeltaSeconds);
    }
}

bool AGhostPressurePlate::IsActorOnPlate(const AActor* ActorToCheck) const
{
    if (!IsValid(ActorToCheck))
    {
        return false;
    }
    // Use component space, respecting the box's rotation and scale exactly once.
    const FVector Point = ActivationBox->GetComponentTransform().InverseTransformPosition(
        ActorToCheck->GetActorLocation());
    const FVector Extent = ActivationBox->GetUnscaledBoxExtent();
    return FMath::Abs(Point.X) <= Extent.X
        && FMath::Abs(Point.Y) <= Extent.Y
        && FMath::Abs(Point.Z) <= Extent.Z;
}

void AGhostPressurePlate::UpdateOccupancy(const AActor* Player, const AActor* Ghost)
{
    bOccupied = IsActorOnPlate(Player) || IsActorOnPlate(Ghost);
    if (ActiveLight->IsVisible() != bOccupied)
    {
        ActiveLight->SetVisibility(bOccupied);
    }
    if (IsValid(GateRef))
    {
        // No latched state: restart/undo derives the gate state from new positions.
        GateRef->SetOpen(bOccupied);
    }
}
