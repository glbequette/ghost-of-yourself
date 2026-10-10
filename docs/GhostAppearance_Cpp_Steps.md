# Translucent player echoes

The ghost now copies the possessed character's skeletal mesh and records its parent-local bone poses alongside the existing timestamped actor transforms. Playback interpolates the poses and holds the final pose. Collision remains disabled. No animation Blueprint needs to be assigned to the ghost.

Echo colors are blue, yellow, then green. `SetEchoNumber(1/2/3)` chooses the tint. The current controller still supports one saved ghost and assigns blue; this change does not enable multiple recordings.

## 1. Full build with Unreal closed

Save your level and close Unreal. In normal PowerShell run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\Rebuild-GhostExitZone.ps1'
```

Wait for the message confirming the build and all three tests passed. If it fails, keep Unreal closed and share the error. This command also runs the expanded recording test for pose interpolation, endpoint/restart behavior and palette order.

## 2. Create the material in the editor

Reopen the project. Ensure the **Python Editor Script Plugin** is enabled under Edit > Plugins. If you enable it now, restart when prompted.

Open Tools > Debug > Output Log. Switch its command input to Python, then enter:

```python
exec(open(r'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\create_ghost_material.py').read())
```

Alternatively, in the normal console command input use:

```text
py "C:/Users/glbeq/Documents/Codex/2026-10-10/continue-development-of-my-36-hour/outputs/create_ghost_material.py"
```

The script creates and saves only `/Game/GhostOfYourself/Materials/M_GhostEcho`. If that asset already exists, it preserves it. The material is unlit, translucent and two-sided, with a stronger silhouette at the edges. The native ghost finds it automatically on spawn and applies a dynamic tint to every material slot.

No map replacement is needed if your controller's **Ghost Class** is the native **RecordedGhost**. Check this in the placed GhostReplayController's Details. A custom class that replaces the native visual components may need its own hookup.

## 3. Test in L_TestRoom

1. Press Play, then R. Wait for the reset fade to finish.
2. Walk a short route, turn, stop on the pressure plate, then press R to save.
3. Observe the blue translucent player body: feet should follow the floor, poses should follow the recording, and the ghost should hold its endpoint.
4. Walk through the ghost. It should not block you. The pressure plate should still respond and the gate should still open.
5. Press X: movement and poses should restart together. Press Z: the ghost should disappear.
6. Complete the room: the old ghost should be removed as before.

If the ghost uses normal opaque player materials, check that M_GhostEcho exists at the exact path above, stop Play, and record again. If you see the fallback cube, the possessed character has no skeletal mesh assigned to its inherited Mesh component; inspect BP_FirstPersonCharacter > Mesh first.

To tune the effect, open M_GhostEcho. `Opacity` defaults to 0.18, `EdgeOpacity` to 0.42 and `EmissiveStrength` to 1.5. The dynamic `EchoColor` parameter is controlled by C++ at runtime.

## Verification status

UnrealHeaderTool passed and GhostRecording.cpp, RecordedGhost.cpp, GhostReplayController.cpp and GhostRecordingTests.cpp compiled to scratch object files. The module has not been linked with these changes, the expanded automation test has not run, and the editor material script has not been executed. The full build above and the visual Play test remain required. No binary map or asset was edited by Codex.
