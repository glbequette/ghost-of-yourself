#include "../GhostLabMusic.h"
#include "../GhostReplayController.h"
#include "../GhostRoomNarration.h"
#include "../GhostUserSettings.h"
#include "Engine/World.h"
#include "Sound/SoundWave.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostLabMusicTest, "GhostOfYourself.Audio.LabMusic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGhostLabMusicTest::RunTest(const FString& Parameters)
{
    UGhostUserSettings* Settings = GetMutableDefault<UGhostUserSettings>();
    const float PreviousMusic = Settings->MusicVolume;
    Settings->MusicVolume = 1.f;
    ON_SCOPE_EXIT { Settings->MusicVolume = PreviousMusic; };
    const UWorld::InitializationValues Init = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false).CreateFXSystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None,
        nullptr, true, ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("World"), World)) { return false; }
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    AGhostReplayController* Replay = World->SpawnActor<AGhostReplayController>();
    if (!TestNotNull(TEXT("Replay"), Replay)) { return false; }
    UGhostLabMusic* Music = Replay->LabMusic;
    UGhostRoomNarration* Voice = Replay->RoomNarration;
    if (!TestNotNull(TEXT("Automatic music component"), Music)
        || !TestNotNull(TEXT("Narration component"), Voice)) { return false; }
    USoundWave* Track = Cast<USoundWave>(Music->MusicTrack.LoadSynchronous());
    if (!TestNotNull(TEXT("Imported original music"), Track)) { return false; }
    TestTrue(TEXT("Music is configured to loop"), Track->bLooping != 0);
    Voice->SkipRoom(0);
    TestEqual(TEXT("Ambient volume after voice finishes"), Music->GetTargetMusicVolume(), Music->AmbientVolume);
    Voice->ReplayRoom(0);
    TestTrue(TEXT("Queued introduction counts as narration"), Voice->IsNarrationPending());
    TestEqual(TEXT("Music ducks for the entire voice sequence"), Music->GetTargetMusicVolume(), Music->NarrationVolume);
    Voice->SkipRoom(0);
    TestEqual(TEXT("Skipping voice restores ambient target"), Music->GetTargetMusicVolume(), Music->AmbientVolume);
    Voice->ReplayRoom(4);
    TestFalse(TEXT("Room without clips does not duck music"), Voice->IsNarrationPending());
    TestEqual(TEXT("Silent room keeps ambient volume"), Music->GetTargetMusicVolume(), Music->AmbientVolume);
    Music->bMusicEnabled = false;
    TestEqual(TEXT("Music can be muted"), Music->GetTargetMusicVolume(), 0.f);
    return true;
}
#endif
