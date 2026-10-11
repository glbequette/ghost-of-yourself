# Controls, ghost counter, and 400 cm/s walking speed

The shared replay controller now draws a small translucent controls legend in the bottom-right corner. It shows WASD/mouse, Space, E, R, X, and Z during puzzle play. Room 0 shows only movement/look and jump, and rooms with a zero ghost allowance omit the recording control. Developer room cheats are not included in the player legend.

Above the controls, Ghosts: USED / ALLOWED shows the number of saved ghosts and the current room allowance. It updates when saving, undoing, replacing, or changing rooms, and turns amber when full. During replacement recording, the hidden old ghost still occupies its saved slot until the new recording is saved, so the counter stays at the room limit.

The legend is independent of recording wisps and is drawn above them for readability. It does not bring back recording/live role labels.

The existing first-person character's Max Walk Speed is set to 400 cm/s (4 m/s) by the shared controller at initialization and kept at that setting during play. Both recording and live attempts use the same walking cap. No Blueprint character or binary map asset was directly edited.

## Build, validate, and reopen

Save All and close Unreal, then run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\Rebuild-GhostControls.ps1'
```

The script performs a full rebuild, checks all thirteen native tests, then opens the project only after validation succeeds.

## Editor options and checks

No widget hookup is required. Select the ONE shared GhostReplayController:

- Ghost > HUD > Show Controls HUD toggles the controls legend and ghost counter (default on).
- Ghost > Movement > Player Max Walk Speed changes the walking cap (default 400).
- Show Recording Wisps remains independent of the controls legend.

Play and check that the legend is readable at your intended window size, remains visible during recording, and movement is slower in both phases. Retest the timing of your laser crossings and button recordings at the new speed. If BS_LabSubject_Move still places its full walk sample at Speed 600, moving that sample to 400 makes full walking speed use the full walk animation rather than a blend with idle.

## Validation status

UnrealHeaderTool and scratch MSVC compilation passed for the changed controller and HUD test. The test checks default walking speed and its persistence, room-appropriate control hints, counter updates after saving/cancelling and in Room 0, and that controls remain registered with wisps disabled. Full module linking, execution of all thirteen tests, and visual/gameplay verification remain pending the build script.

