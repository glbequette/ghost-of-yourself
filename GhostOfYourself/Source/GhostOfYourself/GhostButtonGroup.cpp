#include "GhostButtonGroup.h"
#include "GhostButton.h"
#include "GhostGate.h"
#include "GhostReplayController.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogGhostButtons, Log, All);

AGhostButtonGroup::AGhostButtonGroup()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AGhostButtonGroup::GetButtons(TArray<AGhostButton*>& OutButtons) const
{
    OutButtons.Reset();
    for (TActorIterator<AGhostButton> It(GetWorld()); It; ++It)
    {
        if (IsValid(*It) && It->GroupRef == this) { OutButtons.Add(*It); }
    }
}

bool AGhostButtonGroup::IsButtonUsed(const AGhostButton* Button) const
{
    return IsValid(Button) && UsedButtons.Contains(const_cast<AGhostButton*>(Button));
}

bool AGhostButtonGroup::IsActiveRoom() const
{
    return IsValid(ControllerRef) && IsValid(GateRef)
        && ControllerRef->CurrentRoomNumber == RoomNumber
        && !ControllerRef->bResetInProgress && !GateRef->bLockedClosed;
}

void AGhostButtonGroup::BeginPlay()
{
    Super::BeginPlay();
    if (IsValid(ControllerRef))
    {
        AddTickPrerequisiteActor(ControllerRef);
        ControllerRef->OnAttemptReset.AddUObject(this, &AGhostButtonGroup::ResetPuzzle);
    }
    TArray<AGhostButton*> Buttons;
    GetButtons(Buttons);
    if (!IsValid(ControllerRef) || !IsValid(GateRef) || Buttons.IsEmpty())
    {
        UE_LOG(LogGhostButtons, Warning, TEXT("%s needs Controller Ref, Gate Ref, and at least one GhostButton with Group Ref pointing here."), *GetName());
    }
    ResetPuzzle();
}

void AGhostButtonGroup::ExpirePresses(float Time)
{
    for (auto It = RecentPresses.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid() || It.Key()->GroupRef != this
            || Time - It.Value() > FMath::Max(.05f, SyncWindowSeconds) + UE_SMALL_NUMBER)
        {
            It.RemoveCurrent();
        }
    }
}

bool AGhostButtonGroup::ReceivePress(AGhostButton* Button, float Time, const FVector& ActorPosition)
{
    if (!IsActiveRoom() || bUnlocked || !IsValid(Button) || Button->GroupRef != this
        || !Button->IsInRange(ActorPosition) || !FMath::IsFinite(Time) || IsButtonUsed(Button)) { return false; }
    TArray<AGhostButton*> Buttons;
    GetButtons(Buttons);
    if (Buttons.IsEmpty()) { return false; }
    ExpirePresses(Time);
    UsedButtons.Add(Button);
    RecentPresses.Add(Button, Time);
    if (RecentPresses.Num() == Buttons.Num())
    {
        bUnlocked = true;
        GateRef->SetOpen(true);
        UE_LOG(LogGhostButtons, Display, TEXT("Room %d buttons synchronized."), RoomNumber);
    }
    for (AGhostButton* Member : Buttons)
    {
        Member->SetFeedback(RecentPresses.Contains(Member), bUnlocked, IsButtonUsed(Member));
    }
    return true;
}

void AGhostButtonGroup::ResetPuzzle()
{
    RecentPresses.Reset();
    UsedButtons.Reset();
    bUnlocked = false;
    if (IsValid(GateRef)) { GateRef->SetOpen(false); }
    TArray<AGhostButton*> Buttons;
    GetButtons(Buttons);
    for (AGhostButton* Button : Buttons) { Button->SetFeedback(false, false); }
}

void AGhostButtonGroup::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (IsValid(ControllerRef)) { ExpirePresses(ControllerRef->Clock); }
    TArray<AGhostButton*> Buttons;
    GetButtons(Buttons);
    for (AGhostButton* Button : Buttons) { Button->SetFeedback(RecentPresses.Contains(Button), bUnlocked, IsButtonUsed(Button)); }
    if (IsValid(GateRef))
    {
        GateRef->SetOpen(bUnlocked);
        GateRef->AdvanceMovement(DeltaSeconds);
    }
    if (GEngine && bShowDebugStatus && IsActiveRoom())
    {
        float Remaining = 0.f;
        if (RecentPresses.Num() > 0)
        {
            for (const auto& Press : RecentPresses)
            {
                const float ThisRemaining = FMath::Max(0.f, SyncWindowSeconds - (ControllerRef->Clock - Press.Value));
                Remaining = Remaining > 0.f ? FMath::Min(Remaining, ThisRemaining) : ThisRemaining;
            }
        }
        GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.f,
            bUnlocked ? FColor::Green : FColor::Yellow,
            bUnlocked ? TEXT("Buttons synchronized | Gate unlocked until X/Z reset")
                : FString::Printf(TEXT("Buttons %d/%d | %.1fs remaining | E presses a nearby panel"), RecentPresses.Num(), Buttons.Num(), Remaining));
    }
}

void AGhostButtonGroup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(ControllerRef)) { ControllerRef->OnAttemptReset.RemoveAll(this); }
    if (GEngine) { GEngine->RemoveOnScreenDebugMessage(static_cast<uint64>(GetUniqueID())); }
    Super::EndPlay(EndPlayReason);
}
