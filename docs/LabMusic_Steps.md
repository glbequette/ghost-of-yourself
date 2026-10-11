# Lab music

Original soft futuristic electronic pulse: GOY_LabMusic.wav, 90 BPM, stereo 48 kHz,
42.67 seconds. Synthesized specifically for this project, with no sampled recordings.
Source and generator are in C:/Dev/GhostOfYourself/SourceArt/Music.

1. Save All and close Unreal. Do not use Live Coding for this change.
2. Run outputs/Rebuild-GhostMusic.ps1 from this chat folder. It builds the editor
   module, imports the WAV into /Game/GhostOfYourself/Audio/GOY_LabMusic,
   enables looping, runs all fifteen native tests, and opens the project only if
   every check succeeds. Python plugins are enabled for the import command only.
   Keep the script beside its Music folder.
3. Open L_TestRoom. Select the single shared GhostReplayController. Its new
   LabMusic component starts music automatically once the player is possessed.
   No extra music actor or Blueprint wiring is needed.
4. Press Play. Listen in Room 0, then advance to Room 1. The music should continue
   across room changes and R/X/Z resets, quietly underneath the robot narration.
5. Record a ghost and cancel it; verify the music continues without restarting.
   Stop Play, Save All, and test again from L_Title.

For volume, select the controller's LabMusic component. Ambient Volume defaults
to 0.22; Narration Volume is 0.055. Music fades in over 1.5 seconds and smoothly
ducks throughout each voice sequence, including the gaps between clips.
Disable Music Enabled to mute it, or assign a different looping SoundWave to
Music Track. Music currently plays in the gameplay lab; the title screen stays silent.

The importer preserves existing GOY_LabMusic audio content if already imported;
it only ensures that Looping is enabled and saves that asset. To replace the
audio later, use Unreal's Reimport action or choose another Music Track.

The project packaging configuration now includes /Game/GhostOfYourself/Audio
in Additional Asset Directories to Cook. Test the packaged game's music and
voice audio before submitting.

Verification completed here: Unreal Header Tool, separate MSVC compilation of
the affected C++ files and new test, Python syntax checks, and WAV clipping/loop
boundary checks. Full editor linking, asset import, automation execution, and
listening in-game are performed by the rebuild script and your Play test.
