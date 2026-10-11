# Replace the oldest ghost when recording slots are full

R now starts a new recording even when the room's ghost allowance is full. The oldest ghost is hidden and suspended while you record its replacement: it does not move, press buttons, hold plates, or block beams. Newer ghosts continue replaying normally. Its saved recording is retained until you save successfully. Once you save with R, the oldest saved ghost is retired and the new recording is appended as the newest ghost. Automatic saving at 20 seconds uses the same rule.

Cancelling with X restores the hidden ghost and preserves all existing recordings. If spawning the new ghost fails, existing recordings are also preserved. A room allowance of 0 still disables recording. If you lowered the room allowance during Play, a successful save removes enough oldest tracks to fit the new allowance.

Surviving ghosts retain their actors, recordings, exact button events, and colors. The new ghost uses a free color slot. All tracks rewind together after saving. Z continues to remove the newest saved ghost, rather than the oldest. Room transitions and developer jumps clear all ghosts as before.

## Build, validate, and reopen

Save All and close Unreal, then run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\Rebuild-GhostReplacement.ps1'
```

This performs a full rebuild, verifies all thirteen native tests, and opens the project only after validation succeeds. No editor hookup changes are required.

## Test it

In a room with an allowance of 2, record route A, then route B. Press R again and record route C. Saving should leave routes B and C; A disappears. Try another recording: B must disappear while C stays visible and active. Cancel with X: B must return and both recordings remain. Try a recording that reaches 20 seconds: the oldest should be replaced automatically. After any replacement, Z should remove the newest ghost.

The same behavior applies to one-slot rooms: saving a second recording replaces their only ghost. The wispy recording effect still appears while recording and fades away after saving or cancelling.

## Validation status

UnrealHeaderTool and scratch MSVC compilation passed for the updated controller, updated multi-ghost test, and new GhostOfYourself.Recording.ReplaceOldest test. Checks cover hiding the oldest ghost, excluding it from plate/beam queries and button playback, restoring visibility on cancel, full-capacity recording, cancel preservation, oldest retirement, newer-track promotion, button event preservation, stable/distinct colors, automatic replacement, newest-first undo, disabled recording, and one-slot replacement. Full module linking and execution of the thirteen tests remain pending the build script. No binary map or asset files were directly edited.

