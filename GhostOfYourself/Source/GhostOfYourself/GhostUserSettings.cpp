#include "GhostUserSettings.h"
void UGhostUserSettings::ClampValues()
{
    MouseSensitivity = FMath::Clamp(MouseSensitivity, .1f, 3.f);
    MusicVolume = FMath::Clamp(MusicVolume, 0.f, 1.f);
    NarratorVolume = FMath::Clamp(NarratorVolume, 0.f, 1.f);
    MasterVolume = FMath::Clamp(MasterVolume, 0.f, 1.f);
}
FInputActionValue UGhostMouseSensitivityModifier::ModifyRaw_Implementation(
    const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime)
{
    CurrentValue *= FMath::Clamp(GetDefault<UGhostUserSettings>()->MouseSensitivity, .1f, 3.f);
    return CurrentValue;
}
