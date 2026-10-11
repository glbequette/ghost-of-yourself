#pragma once
#include "CoreMinimal.h"
#include "InputModifiers.h"
#include "GhostUserSettings.generated.h"

UCLASS(Config=GameUserSettings)
class GHOSTOFYOURSELF_API UGhostUserSettings : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(Config) float MouseSensitivity = 1.f;
    UPROPERTY(Config) float MusicVolume = 1.f;
    UPROPERTY(Config) float NarratorVolume = 1.f;
    UPROPERTY(Config) float MasterVolume = 1.f;
    void ClampValues();
};

UCLASS()
class GHOSTOFYOURSELF_API UGhostMouseSensitivityModifier : public UInputModifier
{
    GENERATED_BODY()
public:
    virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput,
        FInputActionValue CurrentValue, float DeltaTime) override;
};
