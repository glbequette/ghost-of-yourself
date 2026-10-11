#include "GhostButton.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AGhostButton::AGhostButton()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonMesh"));
    ButtonMesh->SetupAttachment(RootComponent);
    ButtonMesh->SetRelativeScale3D(FVector(.35f, .12f, .35f));
    ButtonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ButtonMesh->SetGenerateOverlapEvents(false);
    ButtonMesh->SetSimulatePhysics(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) { ButtonMesh->SetStaticMesh(Cube.Object); }
    IndicatorLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("IndicatorLight"));
    IndicatorLight->SetupAttachment(RootComponent);
    IndicatorLight->SetRelativeLocation(FVector(0.f, -12.f, 0.f));
    IndicatorLight->SetMobility(EComponentMobility::Movable);
    IndicatorLight->SetIntensity(150.f);
    IndicatorLight->SetAttenuationRadius(100.f);
    IndicatorLight->SetCastShadows(false);
    SetFeedback(false, false);
}

bool AGhostButton::IsInRange(const FVector& ActorPosition) const
{
    return FVector::DistSquared(ActorPosition, ButtonMesh->GetComponentLocation())
        <= FMath::Square(FMath::Max(25.f, InteractionDistance));
}

void AGhostButton::SetFeedback(bool bRecentPress, bool bUnlocked, bool bUsed)
{
    bUsedThisAttempt = bUsed;
    IndicatorLight->SetLightColor(bUnlocked ? FLinearColor::Green
        : bRecentPress ? FLinearColor::Yellow
        : bUsed ? FLinearColor(.15f, .15f, .15f) : FLinearColor(0.02f, .25f, 1.f));
}
