# Buttons can be pressed once per attempt

Each GhostButton now accepts only its first valid press in an attempt, whether pressed by the player or a saved ghost. Further presses are rejected and cannot refresh its timestamp. Expiring the synchronization window does not make the button usable again.

The group still requires all linked buttons within Sync Window Seconds. If the timing is missed, X restarts the attempt and rearms the buttons. Starting/saving/cancelling a recording, undoing a ghost, and changing rooms also reset button state through the existing attempt-reset flow. Saved ghosts press their buttons again on a fresh attempt.

The indicator is blue when ready, yellow while its first press contributes to the sync window, dim gray once that press expires, and green after the group succeeds. Used This Attempt is visible on the button in Details during Play. The first-person interaction selector skips used buttons.

## Build, validate, and reopen

Save All and close Unreal, then run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\Rebuild-GhostButtonsOnce.ps1'
```

This performs a full rebuild, validates all thirteen native tests, and opens the project only after validation succeeds. No new editor references are needed.

The unfinished combined-controller source changes from the cancelled task have been removed. Button groups and pressure-plate groups continue to control separate gates.

## Test it

Press one button and press it again: the second press must do nothing. Let its sync window expire: the button dims and still rejects presses. Press X and try again: it must be ready. During recording, repeated presses on the same button must produce just one saved event. On playback, the ghost presses it once; restarting the attempt lets the same recorded press happen again.

## Validation status

UnrealHeaderTool and scratch MSVC compilation passed for the button, group, replay controller, restored plate group, and updated button automation tests. Tests verify rejection of repeats, no timer refresh, rejection after expiry, rearming on reset, the sync-window boundary, and one recorded event for repeated live presses. Full module linking and execution of all thirteen tests remain pending the PowerShell script. No binary assets or maps were directly edited.
