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
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

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
        ControllerRef->OnAttemptReset.AddUObject(this, &AGhostExitZone::ClearPreviousPosition);
    }
    else
    {
        UE_LOG(LogGhostExit, Warning, TEXT("%s: assign Controller Ref; exit completion is disabled without it."), *GetName());
    }
    if (FromRoomNumber != 10 && !IsValid(GateRef))
    {
        UE_LOG(LogGhostExit, Warning, TEXT("%s: assign Gate Ref; room transition is disabled without it."), *GetName());
    }
    WriteExitDiagnostic(TEXT("Exit loaded"), nullptr);
}

void AGhostExitZone::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
    const bool bCanTrack = IsValid(Player) && IsValid(ControllerRef) && !bCompleted
        && !ControllerRef->bRecording && !ControllerRef->bResetInProgress
        && ControllerRef->CurrentRoomNumber == FromRoomNumber;
    if (!IsValid(Player)) { ClearPreviousPosition(); return; }
    if (!bCanTrack) { ClearPreviousPosition(); }
    const FVector CurrentPosition = Player->GetActorLocation();
    const FVector* Previous = bHasPreviousPosition && PreviousPlayer.Get() == Player
        ? &PreviousPlayerPosition : nullptr;
    if (TryCompleteForActorSwept(Player, Previous)) { ClearPreviousPosition(); return; }
    if (!bCanTrack) { return; }
    PreviousPlayer = Player;
    PreviousPlayerPosition = CurrentPosition;
    bHasPreviousPosition = true;
}

bool AGhostExitZone::TryCompleteForActor(const AActor* ActorToCheck)
{
    return TryCompleteForActorSwept(ActorToCheck, nullptr);
}

void AGhostExitZone::ClearPreviousPosition()
{
    bHasPreviousPosition = false;
    PreviousPlayer.Reset();
}

void AGhostExitZone::EndPlay(const EEndPlayReason::Type Reason)
{
    if (IsValid(ControllerRef)) { ControllerRef->OnAttemptReset.RemoveAll(this); }
    ClearPreviousPosition();
    Super::EndPlay(Reason);
}

bool AGhostExitZone::TryCompleteForActorSwept(const AActor* ActorToCheck, const FVector* PreviousPosition)
{
    const bool bFinalExit = FromRoomNumber == 10;
    if (!IsValid(ActorToCheck)) { return false; }
    // Compare identity, rather than treating every Character/ghost as a player.
    const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (!IsValid(Player) || ActorToCheck != Player)
    {
        return false;
    }
    auto Reject = [this, Player](const TCHAR* Reason)
    {
        if (FVector::DistSquared(Player->GetActorLocation(), ExitBox->GetComponentLocation()) <= FMath::Square(350.f))
        { WriteExitDiagnostic(Reason, Player); }
        return false;
    };
    if (bCompleted) { return false; }
    if (!IsValid(ControllerRef)) { return Reject(TEXT("Missing replay controller reference")); }
    if (ControllerRef->bRecording) { return Reject(TEXT("Recording still active")); }
    if (ControllerRef->bResetInProgress) { return Reject(TEXT("Reset in progress")); }
    if (ControllerRef->CurrentRoomNumber != FromRoomNumber) { return Reject(TEXT("Current room does not match exit FromRoomNumber")); }
    if (!bFinalExit)
    {
        if (!IsValid(GateRef)) { return Reject(TEXT("Missing gate reference")); }
        if (GateRef->bLockedClosed) { return Reject(TEXT("Referenced gate is locked")); }
        if (!GateRef->AllowsExitForPlayer(Player)) { return Reject(TEXT("Referenced door is closed or no longer clears player")); }
    }
    const FVector Point = ExitBox->GetComponentTransform().InverseTransformPosition(
        ActorToCheck->GetActorLocation());
    const FVector Extent = ExitBox->GetUnscaledBoxExtent();
    const bool bInside = FMath::Abs(Point.X) <= Extent.X && FMath::Abs(Point.Y) <= Extent.Y
        && FMath::Abs(Point.Z) <= Extent.Z;
    bool bCrossed = false;
    if (!bInside && PreviousPosition)
    {
        const FVector Start = ExitBox->GetComponentTransform().InverseTransformPosition(*PreviousPosition);
        bCrossed = FMath::LineBoxIntersection(FBox(-Extent, Extent), Start, Point, Point - Start);
    }
    if (!bInside && !bCrossed)
    {
        return Reject(TEXT("Player position and movement do not intersect exit box"));
    }
    // Intermediate rooms must clear the door before locking it. The final box
    // is the finish trigger itself and does not depend on door geometry.
    if (!bFinalExit && !GateRef->IsPlayerBeyondGate(Player, ExitBox->GetComponentLocation()))
    {
        return Reject(TEXT("Player capsule has not cleared referenced gate"));
    }
    if (!bFinalExit && IsValid(NextRoomStart) && !GateRef->IsPositionBeyondGate(
        NextRoomStart->GetActorLocation(), ExitBox->GetComponentLocation(),
        Player->GetCapsuleComponent()->GetScaledCapsuleRadius()))
    {
        return Reject(TEXT("Next-room checkpoint is behind or too close to referenced gate"));
    }
    if (!ControllerRef->AdvanceToNextRoom(NextRoomStart))
    {
        return Reject(TEXT("Replay controller rejected room advance"));
    }
    bCompleted = true;
    if (IsValid(GateRef))
    {
        // Avoid closing a final door through a player when its box straddles it.
        if (!bFinalExit || GateRef->IsPlayerBeyondGate(Player, ExitBox->GetComponentLocation()))
        {
            GateRef->LockClosed();
        }
    }
    UE_LOG(LogGhostExit, Display, TEXT("Room %d completed through %s; echo cleared."), FromRoomNumber, *GetName());
    WriteExitDiagnostic(TEXT("Exit completed"), Player);
    return true;
}

void AGhostExitZone::WriteExitDiagnostic(const TCHAR* Reason, const ACharacter* Player)
{
    // Shipping builds suppress normal engine logs. Keep a small separate trace
    // for this packaging issue, without adding debugging text to the game's HUD.
    if (FApp::IsUnattended()) { return; }
    const int32 CurrentRoom = IsValid(ControllerRef) ? ControllerRef->CurrentRoomNumber : -1;
    const FString Key = FString::Printf(TEXT("%s:%d"), Reason, CurrentRoom);
    if (LastDiagnosticReason == Key) { return; }
    LastDiagnosticReason = Key;
    const FString Line = FString::Printf(TEXT("%s | exit=%s from=%d current=%d | %s | controller=%s gate=%s open=%d locked=%d | player=%s box=%s extent=%s checkpoint=%s position=%s\n"),
        *FDateTime::Now().ToString(), *GetName(), FromRoomNumber, CurrentRoom, Reason,
        *GetNameSafe(ControllerRef), *GetNameSafe(GateRef), IsValid(GateRef) && GateRef->bIsOpen,
        IsValid(GateRef) && GateRef->bLockedClosed,
        Player ? *Player->GetActorLocation().ToString() : TEXT("none"),
        *ExitBox->GetComponentLocation().ToString(), *ExitBox->GetScaledBoxExtent().ToString(),
        *GetNameSafe(NextRoomStart), IsValid(NextRoomStart) ? *NextRoomStart->GetActorLocation().ToString() : TEXT("none"));
    const FString Directory = FPaths::ProjectSavedDir() / TEXT("Logs");
    IFileManager::Get().MakeDirectory(*Directory, true);
    FFileHelper::SaveStringToFile(Line, *(Directory / TEXT("GhostExitTransitions.log")),
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
}
