#pragma once
#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "GhostRoomNarration.generated.h"

USTRUCT(BlueprintType)
struct FGhostRoomVoicePlaylist
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ghost|Narration")
    TArray<TObjectPtr<USoundBase>> Clips;
};

// Plays room tutorials on a real-time clock, independently of recording/reset clocks.
UCLASS(ClassGroup=(Ghost), meta=(BlueprintSpawnableComponent))
class GHOSTOFYOURSELF_API UGhostRoomNarration : public UAudioComponent
{
    GENERATED_BODY()
public:
    UGhostRoomNarration();
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTick) override;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ghost|Narration")
    TMap<int32, FGhostRoomVoicePlaylist> RoomPlaylists;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ghost|Narration", meta=(ClampMin="0", Units="s"))
    float RoomStartDelay = .75f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ghost|Narration", meta=(ClampMin="0", Units="s"))
    float ClipGap = .35f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Ghost|Narration")
    int32 NarrationRoom = INDEX_NONE;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Ghost|Narration")
    int32 ClipIndex = 0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Ghost|Narration")
    bool bSequenceFinished = false;
    bool HasFinishedRoom(int32 Room) const { return NarrationRoom == Room && bSequenceFinished; }
    bool IsNarrationPending() const;
    void ReplayRoom(int32 Room);
    void SkipRoom(int32 Room);
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void StartNextClip();
    void AdvanceClip(UAudioComponent* Finished);
    void StopSafely();
    TSet<int32> HeardRooms;
    float WaitRemaining = 0.f;
    float ClipElapsed = 0.f;
    float ClipDuration = 0.f;
    bool bClipPlaying = false;
    bool bStopping = false;
};
