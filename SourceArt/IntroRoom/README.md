# Starting room and robot introduction

The player starts in a small enclosed welcome chamber. A friendly but suspicious robot voice introduces the Echo Research Facility and the recording premise. After the 42.5-second introduction, the door slides up. Walking beyond it fades out and loads the existing L_TestRoom at its existing PlayerStart. The character, puzzle rooms, one-ghost recording limit and checkpoint behavior stay in their current level.

Enter skips narration and opens the door. The player can walk/look during the introduction. The intro level contains no replay controller, plates or synchronized buttons, so recording starts with the first assessment. Narration plays once per fresh intro-level load. Reloading the intro or starting the game again plays it again.

## 1. Full C++ build

Save All in Unreal and close it. Run in normal PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\Rebuild-GhostIntro.ps1'
```

This rebuilds and runs all five native tests, including GhostOfYourself.Intro.Flow. Wait for the success message. If it fails, leave Unreal closed and share the first error. Do not use Live Coding.

## 2. Create the new room inside Unreal

1. Reopen the project. Stop Play and **Save All** first.
2. Ensure Python Editor Script Plugin and Editor Scripting Utilities are enabled; restart if you just enabled them.
3. Open Output Log. In its normal console command input, enter:

```text
py "C:/Users/glbeq/Documents/Codex/2026-10-10/continue-development-of-my-36-hour/outputs/IntroRoom/create_intro_room.py"
```

The script imports the robot WAV with timed subtitles and creates `/Game/GhostOfYourself/Maps/L_Intro`. It opens/saves only this new level. It refuses if there are unsaved map/content packages or if L_Intro already exists, preserving your work. It does not modify L_TestRoom or project startup settings.

The room includes a floor, four walls, an enclosed ceiling, a passage behind the doorway, two lights, PlayerStart, one GhostGate and one GhostIntroController. It reuses your imported gate panel/frame, console, cabinet and lab materials when found at their current paths; missing decorative assets fall back to the native cube geometry.

The scripted frame is decorative NoCollision; surrounding wall pieces block its sides. The gate retains physical moving collision. Only the intro controller drives this gate. Do not assign a plate/button group to it or put a GhostReplayController in L_Intro.

## 3. Test before changing the startup map

1. With L_Intro open, Play using **Default Player Start**, rather than spawning at the current camera position.
2. You should spawn facing the closed introduction door with your current player Blueprint. The robot voice begins after about 0.75 seconds. Check volume and subtitle readability.
3. Let the full introduction play. The door should stay closed until it finishes, then rise smoothly.
4. Walk through the doorway. The player's full capsule must clear it before the trigger starts a fade and loads L_TestRoom.
5. Confirm Room 1 starts with no saved ghost and recording still works. Solve Room 1 and Room 2 as before.
6. Stop Play and start again in L_Intro. Test **Enter** to skip, then walk through. The introduction must not restart while walking around the same session.

If no voice plays, inspect Intro_Controller's **Introduction Audio**: it should reference `/Game/GhostOfYourself/Audio/Intro_RobotWelcome`. The controller opens the door if the file is missing rather than trapping the player. Check Narration volume and game/editor audio output. Enable **Subtitles Enabled** in Project Settings if subtitles are disabled globally.

If the doorway does not load Room 1, inspect **Assessment Level**. It must be the long package name `/Game/GhostOfYourself/Maps/L_TestRoom`, without .umap or an object suffix. Keep the intro actor/ExitBox beyond the door. Its default actor position is `(0,690,0)` with the gate at `(0,500,0)`, so starting inside the welcome chamber cannot trigger travel.

## 4. Make it the game's starting map

After the Play test passes:

1. Open **Edit > Project Settings > Maps & Modes**.
2. Set **Game Default Map → L_Intro**. This controls the level when the game launches without an explicitly selected map.
3. If you also want the editor to open in the welcome room, set **Editor Startup Map → L_Intro**. Otherwise keep your preferred editor map.
4. Keep the existing first-person GameMode. The new room also explicitly uses BP_FirstPersonGameMode, which spawns your current BP_FirstPersonCharacter.
5. Under **Project Settings > Packaging**, add both L_Intro and L_TestRoom to **List of maps to include in a packaged build**. The intro actor's level-name property alone is not a guarantee that the destination gets cooked.
6. Save All, then test a packaged launch. Verify the voice, subtitles, skip, transition and first puzzle. PIE validation does not confirm packaging.

## Voice and story

`Audio/Intro_RobotWelcome.wav` is mono, 48 kHz, 16-bit PCM, normalized below clipping. It uses the locally installed Microsoft Zira Desktop synthetic voice with light delay/modulation for a robot texture. `Narration.json` contains the measured cue times; `Story_Script.txt` is the editable story text. This is a synthesized prototype voice, not a recording of an actor.

The opening establishes that the player supposedly volunteered, explains help from their past self, and hints at previous visits:

> “I am delighted you volunteered. Your file says you did.”
>
> “And if any of this feels familiar, please continue as normal. We will handle the remembering.”

To regenerate, use Windows PowerShell to run Source/generate_voice.ps1 with a RawOutputFolder, then run Source/process_voice.py with that folder (Python/NumPy required). The voice generator expects Microsoft Zira Desktop to be installed.

## Verification status

UnrealHeaderTool passed, and GhostIntroController.cpp and GhostIntroTests.cpp compiled to scratch object files. The new test checks the intro/skip latch, player-only exit, full doorway clearance, destination validation, single travel request and cleanup of input locks. It has not run yet. PowerShell/Python syntax and WAV format/clipping checks passed. The editor creation script has not run, the full module build is pending, and startup-map/cooking settings have not been changed. No existing binary asset/map was directly edited or overwritten, and no commit/push was made for this change.
