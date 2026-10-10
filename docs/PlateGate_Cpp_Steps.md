# Native pressure plate and gate

Current workflow: save and close Unreal for every C++ change, rebuild in normal
PowerShell, then reopen. Live Coding is not the chosen workflow for this project.

After this puzzle works, follow [GhostExitZone_Cpp_Steps.md](GhostExitZone_Cpp_Steps.md)
to add a native completion zone beyond the gate.

These actors extend the working one-ghost prototype. No Blueprint graphs are
required. The assistant added source only; you place and link actors in Unreal.

## Sliding gate update

The gate now raises its mesh into the wall instead of disappearing. The latest
Live Coding attempt was followed by a DirectX 12 GPU crash at 11:43 a.m. on
October 10, 2026. The code is still saved, but the motion update has not been
verified in a successful build. For recovery, save and close Unreal, use the
full build below, run the tests, and reopen the editor. Then select your existing
GhostGate and set Open Height = 270 and Travel Speed = 400 in Details > Ghost >
Gate. Start a fresh Play session. The crash report identifies a GPU failure;
it does not establish that the door code caused it.

Open Height is travel distance in the gate actor's local Z direction, in cm
before actor scale. Travel Speed is cm per second in that space. The default
250 cm door takes about 0.675 seconds to raise by 270 cm. Leave room above the
doorway for the raised door or add a wall header to conceal it; the code does
not change your wall geometry. Adjust Open Height if your door is taller.

The linked pressure plate advances gate motion each frame. Keep exactly one
plate linked to a gate. No gate Tick or constructor setup change is needed.
Collision stays enabled on the moving mesh. X/Z now request closing, and the
door lowers over time. bIsOpen describes the requested open state, not whether
the mesh has already finished raising. Repeated occupancy checks do not reset
the animation; leaving mid-animation reverses it from the current position.

## What the code does

- GhostPressurePlate checks the local player's and its linked controller's
  current GhostRef actor origins inside ActivationBox each frame.
- Collision and overlaps are disabled on the plate and ghost. The activation
  box tests positions in component space, respecting rotation and scale.
- A cyan light comes on while either actor occupies the plate.
- GhostGate slides its visible, collidable cube upward when occupied and
  downward when empty. BlockAllDynamic collision moves with the mesh; physics
  simulation stays off.
- The plate runs after recording/playback updates. X, Z, saving, and canceling
  derive the gate state from the resulting positions on the same frame's plate
  tick. A ghost holding its endpoint continues to occupy the plate.
- One plate controls one gate. Do not link multiple plates to the same gate;
  combined plate requirements are a later mechanic.

## Verification status

The existing recorder previously built successfully and its playback test passed.
You reported that the placed plate/gate works. This sliding-motion update and
the expanded automation checks have not yet been compiled or run. The assistant's
sandbox cannot run the build accelerator's Windows volume query, even with
-NoUBA. Build from normal PowerShell below with Unreal closed.
No binary asset edits, commits, or pushes were made by the assistant.

## 1. Save, close Unreal, and build

For the first addition of the native classes, a full build and restart is recommended.
Save your current editor work first, then close Unreal. Run:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' GhostOfYourselfEditor Win64 Development '-Project=C:\Dev\GhostOfYourself\GhostOfYourself\GhostOfYourself.uproject' -WaitMutex -NoHotReloadFromIDE -NoLiveCoding -NoUBA '-Log=C:\Dev\GhostOfYourself\GhostOfYourself\Saved\Logs\CppBuild.log'
```

Require Result: Succeeded. If it fails, keep Unreal closed and provide the error;
the assistant can read CppBuild.log directly.

## 2. Run both native tests

With Unreal still closed, run:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Dev\GhostOfYourself\GhostOfYourself\GhostOfYourself.uproject' /Engine/Maps/Entry -unattended -nop4 -nosplash -NullRHI -nosound '-ExecCmds=Automation RunTests GhostOfYourself' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=C:\Dev\GhostOfYourself\GhostOfYourself\Saved\Automation\PlateGate' '-abslog=C:\Dev\GhostOfYourself\GhostOfYourself\Saved\Logs\PlateGateAutomation.log'
```

Require both GhostOfYourself.Recording.Playback and
GhostOfYourself.Puzzles.PlateGate to complete with Result={Success}.
The new test creates a transient world and checks live/ghost occupancy,
gate collision and visibility, gradual raising, reversal, motion endpoints,
repeated commands without drift, endpoint holding, restart, destroyed ghosts,
rotated/scaled bounds, boundary inclusion, missing references, and indicator
light state. It does not prove the placed doorway blocks the actual player.

## 3. Place the native actors

1. Reopen C:\Dev\GhostOfYourself\GhostOfYourself\GhostOfYourself.uproject.
2. Open Content/GhostOfYourself/Maps/L_TestRoom. Leave the working native
   GhostReplayController and first-person character setup as they are.
3. Enable Show C++ Classes in Content Browser settings if needed.
4. Under C++ Classes > GhostOfYourself, drag GhostGate into the map. Place
   its actor origin at floor level across a doorway. Its cube is 200 cm wide,
   20 cm thick, and 250 cm tall. Rotate the actor to align with the doorway;
   resize its GateMesh component or actor if needed.
5. Keep a floor through the doorway. Make surrounding walls tall enough and
   the gate wide enough that the template's jumping player cannot bypass it.
6. Drag GhostPressurePlate into the room. Place its actor origin at floor
   level, several steps from the gate. Do not place it at PlayerStart.
7. Select the placed plate. In Details > Ghost > Plate, assign with eyedroppers:
   Gate Ref = the placed GhostGate; Controller Ref = the existing native
   GhostReplayController. Compile no Blueprints; save the map.
8. If you already created BP_Gate/BP_PressurePlate from the earlier instructions,
   remove their level instances when replacing them. Keep their assets if useful.

The default plate is a 200 x 200 cm thin cube. ActivationBox is centered 90 cm
above it, with Box Extent (100,100,100). That includes the character's capsule
center and the recorded ghost's actor origin. If the visual plate is resized,
adjust ActivationBox accordingly; resizing only PlateMesh does not resize the
activation area. Keep the gate's actor collision enabled and its mesh collision
profile BlockAllDynamic. Native SetOpen chooses the motion target; the plate
advances the gate mesh each frame while keeping its collision and visibility on.

## 4. Manual checks

1. Play with one player, then click the viewport. Walk to the closed gate:
   it must block you, including attempts to jump around or over it.
2. Walk onto the plate. Expect cyan light and the gate smoothly sliding up.
   Walk off: light off, gate slides down to its original position and blocks again.
   Step on/off mid-motion: the door should reverse smoothly without jumping.
3. Press R, walk onto the plate, then R to save while centered on it.
4. Wait for the ghost to reach the plate. It should illuminate the plate and
   hold the gate open indefinitely while you walk through the doorway.
5. Press X. If spawn is outside the plate, the gate lowers until the ghost
   arrives again. Press Z while away from the plate: ghost removed, gate lowers.
6. Record a path that crosses the plate and then leaves it. The gate should open
   only for the occupied interval, closing after the ghost leaves.
7. Stand on the plate yourself while the ghost leaves: the gate stays open.
   Leave the plate yourself and it closes. X to cancel an active recording also
   resets occupancy based on the returned player position.
8. Stop Play and restart. Gate begins closed unless an actor occupies the plate.

Output Log warnings naming the plate mean Gate Ref or Controller Ref is missing.
If the ghost doesn't activate it, check Controller Ref and ActivationBox's height
and extents. Do not enable ghost collision to fix occupancy.

This is a graybox plate/gate test with a sliding door. An exit trigger, completion
UI, button interactions, and extra rooms have not been added.
