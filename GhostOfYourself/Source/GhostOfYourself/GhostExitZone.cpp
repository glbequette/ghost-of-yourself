#include "GhostExitZone.h"
#include "GhostReplayController.h"
#include "GhostGate.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Engine/TargetPoint.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogGhostExit, Log, All);

AGhostExitZone::AGhostExitZone()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    ExitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ExitBox"));
    ExitBox->SetupAttachment(RootComponent);
    ExitBox->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
    ExitBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));
    ExitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ExitBox->SetGenerateOverlapEvents(false);
    ExitBox->SetHiddenInGame(true);

    ExitMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExitMarker"));
    ExitMarker->SetupAttachment(RootComponent);
    ExitMarker->SetRelativeLocation(FVector(0.f, 0.f, 2.5f));
    ExitMarker->SetRelativeScale3D(FVector(2.f, 2.f, 0.05f));
    ExitMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ExitMarker->SetGenerateOverlapEvents(false);
    ExitMarker->SetSimulatePhysics(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        ExitMarker->SetStaticMesh(Cube.Object);
    }

    ExitLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ExitLabel"));
    ExitLabel->SetupAttachment(RootComponent);
    ExitLabel->SetRelativeLocation(FVector(0.f, 0.f, 140.f));
    ExitLabel->SetHorizontalAlignment(EHTA_Center);
    ExitLabel->SetWorldSize(40.f);
    ExitLabel->SetTextRenderColor(FColor::Green);
    ExitLabel->SetText(FText::GetEmpty());
    ExitLabel->SetVisibility(false);
    ExitLabel->SetHiddenInGame(true);
}

void AGhostExitZone::BeginPlay()
{
    Super::BeginPlay();
    bCompleted = false;
    // Keep the inherited component for existing placed actors, but remove its text.
    ExitLabel->SetText(FText::GetEmpty());
    ExitLabel->SetVisibility(false);
    ExitLabel->SetHiddenInGame(true);
    if (IsValid(ControllerRef))
    {
        AddTickPrerequisiteActor(ControllerRef);
    }
    else
    {
        UE_LOG(LogGhostExit, Warning, TEXT("%s: assign Controller Ref; exit completion is disabled without it."), *GetName());
    }
    if (!IsValid(GateRef))
    {
        UE_LOG(LogGhostExit, Warning, TEXT("%s: assign Gate Ref; room transition is disabled without it."), *GetName());
    }
}

void AGhostExitZone::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    TryCompleteForActor(UGameplayStatics::GetPlayerCharacter(this, 0));
}

bool AGhostExitZone::TryCompleteForActor(const AActor* ActorToCheck)
{
    if (bCompleted || !IsValid(ControllerRef) || ControllerRef->bRecording || ControllerRef->bResetInProgress
        || ControllerRef->CurrentRoomNumber != FromRoomNumber
        || !IsValid(GateRef) || !GateRef->bIsOpen || GateRef->bLockedClosed
        || !IsValid(ActorToCheck))
    {
        return false;
    }
    // Compare identity, rather than treating every Character/ghost as a player.
    const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (ActorToCheck != Player)
    {
        return false;
    }
    const FVector Point = ExitBox->GetComponentTransform().InverseTransformPosition(
        ActorToCheck->GetActorLocation());
    const FVector Extent = ExitBox->GetUnscaledBoxExtent();
    if (FMath::Abs(Point.X) > Extent.X || FMath::Abs(Point.Y) > Extent.Y
        || FMath::Abs(Point.Z) > Extent.Z)
    {
        return false;
    }
    // Require the player's entire capsule to be past the doorway before locking it.
    if (!GateRef->IsPlayerBeyondGate(Player, ExitBox->GetComponentLocation()))
    {
        return false;
    }
    if (IsValid(NextRoomStart) && !GateRef->IsPositionBeyondGate(
        NextRoomStart->GetActorLocation(), ExitBox->GetComponentLocation(),
        Player->GetCapsuleComponent()->GetScaledCapsuleRadius()))
    {
        return false; // Never adopt a checkpoint behind the newly locked door.
    }
    if (!ControllerRef->AdvanceToNextRoom(NextRoomStart))
    {
        return false;
    }
    bCompleted = true;
    GateRef->LockClosed();
    UE_LOG(LogGhostExit, Display, TEXT("Room %d completed through %s; gate locked and echo cleared."), FromRoomNumber, *GetName());
    return true;
}
