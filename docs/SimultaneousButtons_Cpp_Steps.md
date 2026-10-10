# Room 2: simultaneous buttons

Two distinct buttons must be pressed with E within 1.5 seconds of each other. Each press pulses yellow, then expires. Success turns both lights green and latches the linked gate open until an attempt reset. Walking near a button or leaving a ghost beside it does not press it.

The ghost records the exact button reference and press timestamp and replays each event once on the shared clock. Restart emits the events again. There is still one saved ghost. Blue/yellow/green echo colors are unchanged.

## 1. Build with Unreal closed

Save the level and close Unreal. Run this in normal PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\Rebuild-GhostButtons.ps1'
```

Wait for **Build succeeded and all four native tests passed**. The script runs Recording.Playback, Puzzles.PlateGate, Puzzles.ExitZone and the new Puzzles.Buttons tests. If it fails, leave Unreal closed and share the error. Do not use Live Coding for this change.

## 2. Keep these Room 2 actors

- Keep Room 2's GhostGate and GhostExitZone.
- Keep the single original GhostReplayController shared by both rooms.
- Keep the Room 1 pressure plate and gate.
- Room 2's removed pressure plate stays removed. No plate should control Room 2's gate alongside the button group.

## 3. Add one group

Reopen L_TestRoom. In Place Actors search **Ghost Button Group**, or use C++ Classes > GhostOfYourself > GhostButtonGroup. Drag one into Room 2. Its position is only an organization marker; it has no visible mesh.

Rename its actor label **Room2_ButtonGroup**. In its Details set:

| Property | Value |
| --- | --- |
| Controller Ref | The original shared GhostReplayController |
| Gate Ref | Room 2's GhostGate |
| Room Number | 2 |
| Sync Window Seconds | 1.5 |
| Show Debug Status | Enabled for testing |

Do not add another replay controller.

## 4. Add two buttons

Place **Ghost Button** twice. Rename the labels **Room2_ButtonA** and **Room2_ButtonB**. For both set **Group Ref → Room2_ButtonGroup** and leave **Interaction Distance = 180 cm**.

These buttons include a small cube panel and a feedback light. Their actor origins are the panel centers. Put them roughly **120–140 cm above the floor**, with the panel sitting just in front of the wall. Rotate each actor so its thin local Y axis runs perpendicular to its wall. Place the light on the room-facing side if it initially points into the wall. Keep the actor scale at 1.

The panels need no collision; E checks distance, view direction and visibility rather than overlap. Keep their centers in front of the wall so the visibility trace can reach them. A blocking wall between player and panel prevents live interaction.

Place A and B far enough apart that traveling between their interaction areas takes longer than 1.5 seconds. Start with about **1,000–1,200 cm** between panel centers; measure in Play because character speed and layout determine the actual travel time. Both panels must remain reachable from Room 2's start within about eight seconds.

Exactly two GhostButtons must point to this group. If another button is accidentally assigned to it, the group refuses activation and logs a setup warning at startup.

## 5. Check the exit references

Select Room 2's GhostExitZone:

| Property | Value |
| --- | --- |
| Controller Ref | Original shared GhostReplayController |
| Gate Ref | Room 2's GhostGate |
| From Room Number | 2 |
| Next Room Start | Room 3's floor-level TargetPoint, if it exists; otherwise clear a copied stale reference |

Keep the exit beyond the doorway as before. With no next checkpoint, completion uses the player's actual entry pose as the next recording start. Room 1's exit should still point to Room 2's start TargetPoint.

Save the level. This guide does not change the map automatically.

## 6. Play the puzzle

1. Solve Room 1 and enter Room 2. The controller must display **Room 2**; its old ghost should be gone.
2. Press R. After the reset fade, walk to Button A.
3. At approximately **8.0 seconds** on the shared clock, look at A and tap E. Its light should turn yellow. The group should show **1/2** and a countdown.
4. Press R to save. Saving resets the button state and clock.
5. Walk to B during replay. Look at it and tap E around **8.0 seconds**, within 1.5 seconds of the ghost's press on A.
6. Both lights turn green. The gate moves up and remains open while you leave the buttons.
7. Walk through the gate and exit zone. The ghost is removed and the completed gate locks closed behind you.

Eight seconds is a suggested target, not a rule. You can record a later or earlier press if both routes fit the 20-second recording limit.

## 7. Failure and reset checks

- Press A twice: the gate must stay closed. Two presses on one panel do not count as two buttons.
- Press B more than 1.5 seconds after A: the old press must expire and the gate must stay closed.
- Press X after success: the clock restarts, both lights reset and the gate lowers; the saved ghost presses A again at its original time.
- Wait at the recorded endpoint: there must be no repeated ghost presses.
- Press Z: the ghost, samples and button events are removed, and the gate closes.
- Start a recording, press A, then cancel with X. A later recording without a button press must not inherit that canceled press.
- Walk through the ghost: it must remain nonblocking. Room 1's plate puzzle must still work.

If E does nothing, verify Room Number = 2, the shared controller is in Room 2, the two Group Ref links are correct, the group has its gate/controller assigned, and the panel is nearby and visible. The group does not accept presses during a reset fade or after success until reset.

## Verification status

UnrealHeaderTool passed with the new classes. GhostButton.cpp, GhostButtonGroup.cpp, GhostReplayController.cpp and GhostButtonTests.cpp compile to scratch object files. Full linking, the four automation tests and visual editor hookup remain pending the rebuild and Play steps above. Existing binary maps and assets were not edited.
