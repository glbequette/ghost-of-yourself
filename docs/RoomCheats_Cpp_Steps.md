# Developer room shortcuts

In Play mode with the viewport focused, press 0–9 to jump to that room, or F10 for Room 10. Numpad 0–9 also work with Num Lock on. These shortcuts are disabled in Shipping builds. You can also untick Enable Room Cheats on the shared GhostReplayController.

Each jump discards the current recording and saved ghost, adopts the destination as the new R/X/Z checkpoint, sets the active room number, and uses the normal reset fade. It rearms the selected room's exit and all later exits, unlocking their completed gates so you can test the room progression again. Earlier completed gates stay locked. Returning to Room 0 rearms its narrator and welcome gate. Jumping away stops its narration. Keys pressed during an existing reset fade are ignored; press again after the fade.

## Build and reopen

Save All and close Unreal, then run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\Rebuild-GhostRoomCheats.ps1'
```

The script performs a full build, verifies all eight native tests, then opens the project only if validation succeeds. If a step fails it stops and leaves Unreal closed.

## Assign room destinations

You do not need to create a controller for each room. Select the ONE shared GhostReplayController. Under Ghost > Developer, keep Enable Room Cheats enabled.

The controller automatically reuses:

- Room 1: the linked GhostIntroController's Room One Start.
- Room 2 onward: the preceding GhostExitZone's Next Room Start. For example, the exit with From Room Number 3 supplies Room 4's destination.

For Room 0, add a floor-level TargetPoint in its safe spawning area. In the replay controller's Cheat Room Starts map, click +, set the integer key to 0, and select that TargetPoint as the value.

You can also add explicit entries for any other room. Example: key 4, value your Room4_Start TargetPoint. An explicit entry overrides automatic discovery. A blank or invalid explicit value disables that room's jump until you assign a marker. If there are multiple distinct automatic checkpoints for a room, add an explicit entry to choose the correct one.

Markers go at floor height; the code adds the player's capsule half-height. Rotate the TargetPoint's yaw to set the player's facing direction. Keep markers out of walls, outside exit boxes, on the correct side of gates, and comfortably away from beams. A missing marker shows a message and does not teleport you or clear the attempt.

Room shortcuts operate on the loaded gameplay map; they do not open a different level or create unfinished rooms. You must build each room and assign its destination before jumping to it.

## Check it

1. Click the Play viewport and press 4. You should fade to Room 4 with no ghost and the HUD showing Room 4.
2. Start a recording with R: it must reset to Room 4, rather than the original spawn point.
3. Complete a room, then jump back to it. Its gate must be able to reopen and its exit must work again.
4. Press 0 to return to the mirror room and restart its introduction.
5. Try a number whose room has no checkpoint: the attempt must remain unchanged.

UnrealHeaderTool and scratch MSVC compilation passed for the changed native sources and new test. Full module linking and execution of the eight tests are pending the build script. No binary assets or maps were directly edited. Editor destination assignment still needs to be done by you.
