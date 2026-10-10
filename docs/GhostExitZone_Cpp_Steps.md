# Exit zone: one-way room transition

Workflow: save and close Unreal before every C++ change, rebuild in normal
PowerShell, then reopen after success. Do not use Live Coding for this project.

## Current behavior

- The floating EXIT/ROOM COMPLETE text is hidden and blank. The existing native
  label component is kept empty for compatibility with already placed actors.
- Only the local first-person player can transition. Ghosts and other characters
  cannot; recording suppresses transitions.
- The player must be inside ExitBox, the linked gate must be open, and the player's
  whole capsule must be past the doorway on the exit's side. An activation box
  overlapping room 1 cannot complete from the room-1 side of the gate.
- On passage, the replay controller destroys the saved ghost, clears the recording
  slot and clock, increments the active room number, and adopts the next checkpoint.
- The old gate locks closed. An invisible collision box blocks return immediately
  while the visible mesh slides down. The old plate cannot reopen a locked gate.
- The player continues walking without a transition teleport. Later R/X/Z resets
  use the next room's fixed start, never the old room's start.
- The exit stays consumed through later recording/restarts. Stop/restart Play
  starts a new session in room 1. Completion/checkpoints are not saved to disk.
- All rooms are in the same map and share one GhostReplayController. This does not
  create room-2 geometry or puzzle mechanics, load another map, or add buttons.

## Verification

UnrealHeaderTool accepted the changed reflected headers with warnings treated as
errors. The changed gate, exit, controller, and exit-test sources compiled with
MSVC using Unreal's response files into scratch objects. The project module DLL
has not been rebuilt with these objects; automation and visible behavior are
pending the full rebuild below. No binary assets, commits, or pushes were made
by the assistant.

The exit automation test remains enabled, per your clarification that removing
"test" meant the floating text. It now checks ghost removal, room/checkpoint
advancement, doorway clearance, wrong-room/missing-reference rejection, consumed
exits, locked-gate reopening prevention, immediate return collision, and new-room
R/X/Z behavior. The original playback and plate/gate tests are also retained.

## 1. Rebuild and run the tests

Run the supplied Rebuild-GhostExitZone.ps1 in normal PowerShell with Unreal saved
and closed. It forces a full rebuild and requires fresh successful reports for:

- GhostOfYourself.Recording.Playback
- GhostOfYourself.Puzzles.PlateGate
- GhostOfYourself.Puzzles.ExitZone

Build log: GhostOfYourself/Saved/Logs/CppBuild.log.
Test log: GhostOfYourself/Saved/Logs/GhostExitAutomation.log.
Report: GhostOfYourself/Saved/Automation/GhostExitZone/index.json.

## 2. Update the existing exit

1. Reopen the nested project and open L_TestRoom. Keep the existing plate, gate,
   first-person player setup, and single GhostReplayController.
2. Select the exit zone. In Details > Ghost > Exit, assign:

| Property | Value |
| --- | --- |
| Controller Ref | Existing GhostReplayController |
| Gate Ref | Room 1's gate that the player just passed through |
| From Room Number | 1 |
| Next Room Start | Optional TargetPoint in room 2, described below |

3. Keep the exit zone beyond the doorway; the default box extends 100 cm each
   way from its center. Place its center about 200 cm beyond the door as a starting
   point, then inspect the ExitBox bounds. Keep the actor origin at floor level.
4. Add a Target Point actor from Place Actors. Name it TP_Room2Start. Put it at
   floor level in a clear area in room 2, entirely past the old gate. Its yaw sets
   the reset facing direction. Leave its scale at 1. Do not add another PlayerStart
   or GhostReplayController to establish this checkpoint.
5. Assign TP_Room2Start to the exit's Next Room Start property. The controller adds
   the player's capsule half-height to this floor marker when calculating resets.
   A marker behind the old door is rejected, so it cannot trap a reset player.
6. If Next Room Start is left empty, the player's exact position/view when entering
   the zone becomes the new fixed start. A marker gives a predictable authored start.
7. If room 2 already has another plate/gate, connect that plate to its own gate and
   the same replay controller. A later room-2 exit should use From Room Number = 2,
   Gate Ref = room-2 gate, and an optional room-3 TargetPoint. No extra rooms are
   built by this code update.
8. Save the map. Keep gates upright; rotate about Z to align their thin local Y
   axis across the doorway. Size GateMesh to span the opening. LockBarrier derives
   its shape from the closed GateMesh and should not need manual resizing.

## 3. Playtest

1. Record an attempt ending on room 1's plate and save it. Let the ghost raise the gate.
2. Walk through the gate into the exit zone. The floating text should be gone.
3. Expect a brief room-2-ready notification; the recorder status should show
   Room 2, Exploring, Echo 0/1. The old ghost disappears and room 1's gate lowers.
4. Immediately try walking back: the doorway should block return even before
   the visible door has finished lowering. The old plate cannot reopen the gate.
5. Press R. You should reset to TP_Room2Start and start a fresh recording.
6. Press X to cancel, then Z. Both return you to the room-2 checkpoint, not room 1.
   The old gate remains locked and the old exit cannot advance the room again.
7. Save a new short route in room 2. One new ghost should replay normally.
8. Stop and restart Play to confirm a new session starts in room 1 with no ghost.

If the transition does nothing, check both required references, From Room Number,
that the gate is open, and that the exit/start marker are beyond the door. Missing
references log warnings. An exit centered on the door's plane or a checkpoint
behind it cannot transition. Keep the player fully beyond the frame before the
zone activates, and ensure adjacent walls prevent walking around the gate.
