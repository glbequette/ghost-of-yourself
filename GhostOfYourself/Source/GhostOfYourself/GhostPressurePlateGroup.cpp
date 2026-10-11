#include "GhostPressurePlateGroup.h"
#include "GhostPressurePlate.h"
#include "GhostReplayController.h"
#include "GhostGate.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"

AGhostPressurePlateGroup::AGhostPressurePlateGroup()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AGhostPressurePlateGroup::GetPlates(TArray<AGhostPressurePlate*>& OutPlates) const
{
    OutPlates.Reset();
    for (TActorIterator<AGhostPressurePlate> It(GetWorld()); It; ++It)
    {
        if (IsValid(*It) && It->GroupRef == this) { OutPlates.Add(*It); }
    }
}

void AGhostPressurePlateGroup::BeginPlay()
{
    Super::BeginPlay();
    if (IsValid(ControllerRef)) { AddTickPrerequisiteActor(ControllerRef); }
    TArray<AGhostPressurePlate*> Plates;
    GetPlates(Plates);
    if (!IsValid(ControllerRef) || !IsValid(GateRef) || Plates.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s needs Controller Ref, Gate Ref, and at least one pressure plate linked through Group Ref."), *GetName());
    }
}

void AGhostPressurePlateGroup::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    TArray<AGhostPressurePlate*> Plates;
    GetPlates(Plates);
    TotalPlateCount = Plates.Num();
    OccupiedPlateCount = 0;
    // Refresh all members together after playback; individual actor tick order cannot leave stale occupancy.
    for (AGhostPressurePlate* Plate : Plates)
    {
        Plate->UpdateCurrentOccupancy();
        if (Plate->bOccupied) { ++OccupiedPlateCount; }
    }
    const bool bActiveRoom = IsValid(ControllerRef) && IsValid(GateRef)
        && ControllerRef->CurrentRoomNumber == RoomNumber
        && !ControllerRef->bResetInProgress && !GateRef->bLockedClosed;
    bAllOccupied = bActiveRoom && TotalPlateCount > 0 && OccupiedPlateCount == TotalPlateCount;
    if (IsValid(GateRef))
    {
        GateRef->SetOpen(bAllOccupied);
        GateRef->AdvanceMovement(DeltaSeconds);
    }
}
