# Reliable exit crossing

The saved map inspection shows GhostExitZone3 uses FromRoomNumber 3, the shared
controller, GhostGate4, and TargetPoint4 on the far side of the gate. Its exit
box is only 50 cm deep. Requiring the entire capsule to clear the gate makes
the valid trigger interval narrower still. Position-only polling can miss this
interval when a frame hitch moves the player across it in one frame.

Native exit ticks now check both the current position and the movement segment
from the preceding frame in the exit box's local space. A crossed box can trigger
once the player's capsule is beyond the open gate. Existing recording, reset,
room, gate, and checkpoint checks remain. Cached movement is cleared after
resets, room changes, recording, and pawn changes, so reset teleports do not
complete exits. Final exits retain their existing box-trigger behaviour.

Run outputs/Rebuild-GhostExitCrossing.ps1 from the chat workspace after Save All
and closing Unreal. It builds and validates all seventeen tests before reopening
the editor. Package Windows Shipping again; the old executable cannot pick up
this source change. Test walking directly through the Room 3 exit, then check
that recording in Room 4 starts from its checkpoint. Replace the downloadable
ZIP only after testing the new package.

The automation regression simulates skipping completely over a 50 cm exit box
in one frame and verifies Room 3 advances to Room 4 once. It also verifies closed
gates, recordings, and reset teleports cannot bypass the exit guards.

UHT and separate MSVC compilation passed here. Full build/test execution and the
packaged-game check remain pending. No map or binary asset was edited.
