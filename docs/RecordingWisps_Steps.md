# Wispy edges while recording a ghost

The recording/live text panel, timer, and red border have been replaced with soft pale-blue wisps around the screen's edges. Two procedural translucent layers slowly drift and breathe. The centre remains transparent so the player can aim at buttons and see the room clearly.

The effect fades in while RECORDING the next ghost, and fades away after saving or cancelling. During the live puzzle attempt the player's view is clear, even while saved ghosts replay. This is the same phase distinction as the previous indicator; the game does not possess a separate ghost pawn.

## Build, validate, and reopen

Save All and close Unreal, then run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\Rebuild-GhostWisps.ps1'
```

The script performs a full rebuild and verifies all thirteen native tests, then opens the project only if validation succeeds. The existing GhostOfYourself.UI.PhaseHUD test now checks the wispy effect's state and HUD lifecycle instead of text labels.

## Check and adjust

No imported texture, material, widget, or GameMode replacement is required. The effect creates its texture in memory, once, and uses the player's HUD.

Select the shared GhostReplayController:

- Ghost > HUD > Show Recording Wisps: enabled by default. This is the renamed display of the existing Show Phase HUD setting.
- Recording Wisp Strength: 0.8 by default. Lower toward 0.4 for a fainter effect; raise toward 1 for a stronger effect.
- Show Debug Status: now defaults off. If your placed controller retained an explicit true value, uncheck it to hide the old technical status text and recording messages. Other button groups have independent debug-status settings.

Press R and confirm wisps fade in once recording begins. Save with R or cancel with X and confirm they fade out. During a second recording, existing ghosts can still replay while the wisps show that YOUR current actions are being recorded. Check different window sizes and make sure the crosshair and central view remain unobstructed.

## Validation status

UnrealHeaderTool and scratch MSVC compilation passed for the updated controller and automation test. The test checks a transparent centre, visible edge opacity, recording fade-in, save/cancel fade-out, clear live play with a replaying ghost, and registration/replacement/cleanup of the HUD. Full module linking, execution of all thirteen tests, and a visual Play-mode check remain pending the build script. No binary assets or map files were directly edited.

