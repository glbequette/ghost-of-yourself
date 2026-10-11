#pragma once
#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "GhostLabMusic.generated.h"

UCLASS(ClassGroup=(Ghost), meta=(BlueprintSpawnableComponent))
class GHOSTOFYOURSELF_API UGhostLabMusic : public UAudioComponent
{
    GENERATED_BODY()
public:
    UGhostLabMusic();
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTick) override;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ghost|Music")
    TSoftObjectPtr<USoundBase> MusicTrack;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ghost|Music", meta=(ClampMin="0", ClampMax="1"))
    float AmbientVolume = .22f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ghost|Music", meta=(ClampMin="0", ClampMax="1"))
    float NarrationVolume = .055f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ghost|Music", meta=(ClampMin="0", Units="s"))
    float StartFadeSeconds = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ghost|Music")
    bool bMusicEnabled = true;
    float GetTargetMusicVolume() const;
    void RefreshSettingsVolume();
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    float CurrentVolume = 0.f;
    bool bAttemptedStart = false;
};
