# Room 6: two ghosts and three simultaneous buttons

The old native group required exactly two buttons; linking a third disabled its presses. Groups now support any positive number of linked buttons and require all members within Sync Window Seconds. Room Number still needs to match the shared controller's current room, and every button must point to the correct group.

Ghost allowances are configured per room using Room Ghost Limits on the shared replay controller. Room 6 initially has allowance 2; unlisted rooms use Default Ghost Limit (initially 1). Blue is the first ghost, yellow the second, green the third, with further hues for additional ghosts. See RoomPuzzleSettings_Steps.md for the current configuration guide.

## Build and reopen automatically

Save All and close Unreal, then run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\Rebuild-GhostMultiple.ps1'
```

The script performs a full rebuild and checks all ten native tests. It reopens the project only if validation succeeds.

## Hook up your three existing buttons

1. Keep ONE GhostReplayController for the entire map. Under Ghost > Multiple Ghosts > Room Ghost Limits, confirm key 6 has value 2.
2. Place or select a separate GhostButtonGroup for Room 6. Set Room Number = 6, Controller Ref = the shared replay controller, and Gate Ref = Room 6's exit gate.
3. Select EACH of the three GhostButtons and set Group Ref to this Room 6 group. For this puzzle these three buttons should point to it; a copied button elsewhere pointing to it would also become required.
4. The button group should be the exit gate's only driver. Remove any plate or beam Gate Ref that also drives this same gate. Other puzzle gates can have their own drivers.
5. Set the group's Sync Window Seconds to 1.5 initially. Use a larger value, such as 2.5, while learning the timing if needed.
6. Make sure entering or using the room cheat changes the HUD to Room 6. A Room 6 group ignores presses while another room is active.
7. Keep buttons near the player's eye level, with their meshes close to their actor origins. Stand within Interaction Distance (180 cm default), look toward the panel, and press E. Walls blocking your view also block interaction.
8. Room 6's exit zone should have From Room Number = 6, this gate, the shared controller, and a safe Room 7 Next Room Start marker.

No binary assets or maps were edited by the assistant; you must assign these actor references in Unreal.

## Record and solve

For the first test, give yourself a clear timing cue: press each button around eight seconds after the attempt starts. Keep each recording shorter than 20 seconds.

1. Press R. Walk to button A, wait until about 8 seconds on the shared clock, look at it, and press E. Press R again to save. A blue ghost now replays that attempt.
2. Press R again to start the second recording. This rewinds the player and blue ghost together; the blue ghost keeps moving and pressing A while you record. Walk to B, press E around 8 seconds, then press R to save. A yellow ghost is added; blue is preserved.
3. Both ghosts rewind to time zero. Walk to C and press E around 8 seconds, when the ghosts press A and B. All three button lights turn green and the gate opens.
4. If timing is difficult, X restarts all ghosts and resets the player to the room checkpoint. The same recorded button events happen again on the shared clock.

The room uses two ghosts plus the live player, not three ghosts. Each recorded button press happens once at its saved timestamp; holding a ghost at its endpoint does not repeatedly press its button.

## Controls and other mechanics

- R adds a recording if a slot is free. At the limit, saving replaces the oldest ghost; X cancels without replacing it. Automatic saving at 20 seconds follows the same rule.
- X cancels the current unfinished recording or restarts the whole attempt; existing ghosts remain.
- Z discards any unfinished recording, removes only the newest saved ghost, and restarts. Press again to remove the remaining ghost.
- Room exits and developer room jumps clear ALL saved ghosts.
- Pressure plates accept either ghost. A security beam stops at the nearest blocking ghost; its emitter side remains dangerous.
- All echoes share the same clock, hold their endpoints, and have no collision or physics. Playback events from different ghosts are processed in timestamp order after slow frames.

## Validation status

UnrealHeaderTool and scratch MSVC compilation passed for all changed native sources, the updated button test, and the new GhostOfYourself.Recording.MultipleGhosts test. Full linking and test execution remain pending the PowerShell script. The new test covers two recordings, playback while recording, three-button synchronization, chronological event merging, slot limits, colors, cancel/restart/undo, second-ghost plate and beam interaction, and clearing every ghost on room exit.


