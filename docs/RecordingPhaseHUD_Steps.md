> Superseded: the labelled panel is replaced by wispy recording edges. See [RecordingWisps_Steps.md](RecordingWisps_Steps.md).

# Recording versus live play

The shared GhostReplayController now draws a persistent bottom-centre status panel.

- Recording: red RECORDING GHOST N / LIMIT label, elapsed/max timer, progress bar, R save / X cancel instructions, and a thin red screen border.
- Live attempt: blue PLAYING AS YOU label, room number, replaying ghost count, and available controls.
- Reset fade: amber PREPARING RECORDING or RESTARTING ATTEMPT label, with a waiting message.

The display follows real controller state, including save, cancellation, automatic saving, multiple ghost slots, room transitions, and developer jumps. It hides the R instruction when the current room has no free ghost slot. Recording still uses the same playable character; the panel explains whether the current actions are being saved for a ghost or used for the live puzzle attempt.

## Build and reopen

Save All and close Unreal, then run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\Rebuild-GhostPhaseHUD.ps1'
```

This performs a full build, validates all twelve native tests, and opens the project only after validation succeeds.

## Editor steps

No widget or GameMode replacement is required. The controller registers with the current player's existing HUD, or creates a default HUD if there is none. Keep the ONE shared controller.

After rebuilding, select it and confirm Ghost > HUD > Show Phase HUD is enabled (the default). For a cleaner screen, you can disable its existing Show Debug Status option; the new phase panel remains visible. Button-group debug status has its own setting.

Play and press R: verify the screen border and label turn red and the timer advances. Press R to save, or X to cancel: verify the label switches to PLAYING AS YOU and the recording border disappears. In a room with several ghost slots, start another recording and check the displayed ghost number advances. Check the display at the viewport/window sizes you intend to use.

## Validation status

UnrealHeaderTool and scratch MSVC compilation passed for the changed controller and GhostOfYourself.UI.PhaseHUD test. The test covers recording/live/reset labels, timer/progress, ghost slot numbers, saving/cancelling, attachment to the existing HUD, duplicate prevention, HUD replacement, and cleanup. Full module linking, execution of all twelve tests, and a visual Play-mode check remain pending. No binary assets or maps were directly edited.

