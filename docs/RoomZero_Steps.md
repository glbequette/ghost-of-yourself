# Room 0: simple mirror material and robot welcome

Room 0 is part of L_TestRoom, before Room 1. BEGIN on the title screen still loads L_TestRoom. Move that level's PlayerStart into Room 0, so the player starts there. The separate L_Intro map is no longer required for this flow.

## Simple reflective material — no rebuild needed

Run this in Unreal's Output Log normal console input:

```text
py "C:/Users/glbeq/Documents/Codex/2026-10-10/continue-development-of-my-36-hour/outputs/create_mirror_material.py"
```

The script creates `/Game/GhostOfYourself/Materials/M_RoomMirror`, using a near-white base color, Metallic 1 and Roughness 0.02. It preserves an existing asset with that name. No scene capture, render target or new mirror actor is used.

Place a thin cube against a Room 0 wall. For a 100 cm default cube, start at scale (0.03,1,2), giving a 3 cm thick, 1 m wide and 2 m tall panel. Rotate it as needed, center it about 1 m above the floor and put the face slightly in front of the wall. Assign M_RoomMirror. Add a simple frame if desired. Use the wall's existing collision or keep the panel's ordinary cube collision.

This material uses the project's existing reflections. Check it in Play: it should look like polished reflective metal, but reflection fidelity and player visibility depend on the renderer. The full-body mesh is Owner No See to keep the camera clear; a simple material does not guarantee that hidden body appears in the reflection. Leave Owner No See enabled rather than reintroducing the camera obstruction.

## Room 0 narration/checkpoint code

Save All, close Unreal and run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\Rebuild-GhostRoomZero.ps1'
```

After successful build/tests, reopen L_TestRoom. Keep one shared GhostReplayController. Select it and set **Current Room Number = 0** in Details. R/X/Z are disabled in Room 0; recording becomes available in Room 1. Keep all Room 1/2 plate/button references pointing to this controller.

1. Finish building Room 0 in this same map, including the reflective panel and a passage into Room 1.
2. Move the level's PlayerStart into Room 0. Keep one intended PlayerStart for this start flow. Test with Default Player Start, not current camera spawn.
3. Add a GhostGate between Room 0 and Room 1. Set Destination Room Number = 1, so its panel displays 01. No plate/button group should control this welcome gate.
4. Add a floor-level TargetPoint inside Room 1, beyond the welcome door. Name it Room1_Start. This becomes the first recording checkpoint.
5. Add a GhostIntroController beyond the welcome gate, where the player walks into Room 1. Its ExitBox should cover the passage after the player's capsule has cleared the gate plane. Avoid overlapping the starting chamber.
6. Set **Gate Ref** to the welcome gate, **Replay Controller Ref** to the shared controller, and **Room One Start** to Room1_Start. Assigning Replay Controller Ref selects same-map Room 0 behavior; Assessment Level is ignored in that mode.
7. Import the existing `IntroRoom/Audio/Intro_RobotWelcome.wav` and assign it as **Introduction Audio**. The native fallback path is `/Game/GhostOfYourself/Audio/Intro_RobotWelcome`. You can add the cue text/times from IntroRoom/Narration.json in the SoundWave's Subtitles settings if you want subtitles. Do not run the old create_intro_room.py script for this same-map arrangement.
8. Save and Play. The robot starts after its short delay, while the player can look/walk. After narration or a fresh Enter press to skip, the gate opens.
9. Walk fully through it. The room counter should become 1 without level travel or teleporting, and the welcome gate locks behind you. Press R and confirm reset goes to Room1_Start rather than back to Room 0.

Keep Room 1's existing GhostExitZone From Room Number = 1, with its Room 2 checkpoint. You do not need a second GhostExitZone for Room 0; its GhostIntroController performs that handoff. No room geometry or placed references were changed automatically.

## Verification

Material script syntax passed; it has not run in Unreal. UnrealHeaderTool and scratch compilation passed for the updated room controller, intro controller and intro test. The test covers recording disabled at room 0, checkpoint required, same-map advancement to room 1, the one-way gate lock and recording enabled at the new checkpoint. Full linking, automation execution, reflective-material appearance and editor hookup remain pending. No existing binary assets/maps were directly edited or overwritten, and no commit/push was made.
