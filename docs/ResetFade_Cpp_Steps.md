# Smooth recording resets

R to start/save, automatic save, X cancel/restart, and Z clear now use a short
black camera fade. Fade out defaults to 0.12 seconds, fade in to 0.18 seconds.
The player resets at full black. Playback clock and sampling remain paused until
the fade finishes; a new recording starts with its first sample at time zero.
Movement/look are temporarily ignored and velocity/jumping stopped during the
transition. Input counters are balanced so another system's existing input lock
is not cleared. Repeated R/X/Z during a fade are ignored.

Walking through a room exit remains continuous; it does not teleport or fade.
Exit transitions are suppressed during a recording reset, preventing a room
completion while waiting to return to the checkpoint. Gate motion can continue
to react to occupancy while the camera is black. No maps or binary assets changed.

## Build and apply

Save and close Unreal. Run your existing Rebuild-GhostExitZone.ps1 from normal
PowerShell. It rebuilds the module and runs all three native tests. The exit test
also checks the reset-fade state machine, paused clocks, input locks, timing of
teleport/recording, and exclusion of fade time from saved samples.

Reopen the project. Select GhostReplayController > Details > Ghost > Transition:

| Property | Default |
| --- | --- |
| Use Reset Fade | Enabled |
| Fade Out Seconds | 0.12 |
| Fade In Seconds | 0.18 |

Try R after exploring elsewhere, then save a five-second route. Expect a quick
fade with the player returning to the checkpoint while black, then the ghost
beginning playback as the view comes back. The route should still take five
seconds; the fade should not add stationary time to the recording.

Check X during recording and replay, Z after saving, and automatic save at 20
seconds. Verify movement/look work after each transition. Try pressing R twice
quickly: the second command is ignored while resetting, so wait until Recording
appears before saving. Check that room-2 resets still use the room-2 checkpoint
and keep room 1 locked. Stop Play during a fade and start again to verify cleanup.

If you want a faster transition, try 0.08 / 0.12 seconds. Disable Use Reset Fade
to restore immediate resets. A missing camera manager also uses immediate reset.
These settings affect presentation, not the 20 Hz sampling interval or ghost limit.

## Validation status

UnrealHeaderTool accepted the new settings with warnings treated as errors.
Changed game sources and the expanded test source compiled to scratch objects
using the installed MSVC toolchain. The module DLL and automation have not yet
been rebuilt/run with this update; visual behavior still needs the editor test.
