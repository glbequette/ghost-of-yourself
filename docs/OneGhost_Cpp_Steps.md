# One ghost: C++ hookup and playtest

Recording resets now include a short camera fade. See
[ResetFade_Cpp_Steps.md](ResetFade_Cpp_Steps.md) for settings and checks.

Current workflow: for every C++ change, save and close Unreal, build in normal
PowerShell, and reopen after success. Do not use Live Coding for this project
for now, per the user's preference after repeated editor crashes.

After the one-ghost checks pass, follow [PlateGate_Cpp_Steps.md](PlateGate_Cpp_Steps.md)
for the native pressure plate and gate. The recorder setup below stays the same.

Scope: L_TestRoom, existing first-person character, one saved ghost, roughly
20 Hz transform samples, 20-second automatic save, interpolated replay, held
endpoint. No plates, buttons, extra rooms, or new character implementation.
This replaces OneGhost_Blueprint_Steps.md. The original plan is design context.

## Current verification

- Installed Build.version confirms Unreal Engine 5.8.3.
- .NET Framework 4.8 SDK registry entry and Include/Lib folders are present.
- Unreal Editor was closed before the build attempts on October 10, 2026.
- Build attempts in the assistant sandbox stopped before game compilation:
  UBA cannot query Windows volumes (`FindFirstVolume failed: Access is denied`).
  In this engine, -NoUBA disables detouring but still creates the UBA executor.
- The normal PowerShell rebuild succeeded after fixing the test source's include
  to ../GhostRecording.h. CppBuild.log reports `Result: Succeeded` (4.48 seconds).
- GhostOfYourself.Recording.Playback ran on October 10, 2026 at 11:08 a.m.
  America/New_York. GhostAutomation.log reports `Result={Success}`, exactly one
  test performed, and an exit request with status 0. The report is saved under
  Saved/Automation/OneGhost. SDK validation warnings for non-Windows platforms
  did not prevent the Windows test from running.
- These checks confirm compilation and playback evaluation math. Visible
  recording/playback and map/input hookup still require the editor playtest.
- Binary assets were not edited. Map hookup and the manual playtest below are
  editor work you must perform. No commit or push was made.

## 1. Build with Unreal closed

1. Save any editor work, then close Unreal. Do not build through Live Coding
   for the initial module/UCLASS hookup.
2. Run this in your normal Windows PowerShell:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' GhostOfYourselfEditor Win64 Development '-Project=C:\Dev\GhostOfYourself\GhostOfYourself\GhostOfYourself.uproject' -WaitMutex -NoHotReloadFromIDE -NoLiveCoding -NoUBA '-Log=C:\Dev\GhostOfYourself\GhostOfYourself\Saved\Logs\CppBuild.log'
```

3. Require `Result: Succeeded` and exit code 0. If it fails, keep the log;
   fix that error before editing the map. The .NET runtime alone is not the
   .NET Framework SDK, but the SDK is now detected on this machine.

## 2. Run the playback automation test

After a successful build, with the interactive editor still closed:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Dev\GhostOfYourself\GhostOfYourself\GhostOfYourself.uproject' /Engine/Maps/Entry -unattended -nop4 -nosplash -NullRHI -nosound '-ExecCmds=Automation RunTests GhostOfYourself.Recording.Playback' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=C:\Dev\GhostOfYourself\GhostOfYourself\Saved\Automation\OneGhost' '-abslog=C:\Dev\GhostOfYourself\GhostOfYourself\Saved\Logs\GhostAutomation.log'
```

Require the named test to appear and complete with `Result={Success}` in
Saved/Logs/GhostAutomation.log; exit code alone is insufficient if no test ran.
The test covers transform evaluation, interpolation, uneven timestamps, slow
frames crossing samples, endpoint holding, rewind, short rotation path, scale,
empty/single-sample recordings, and zero duration. It does not validate key
input, recording transitions, spawning, map wiring, or visible motion in PIE.

## 3. Disable the old Blueprint recording setup

1. Open the nested project:
   C:\Dev\GhostOfYourself\GhostOfYourself\GhostOfYourself.uproject.
2. Open Content > GhostOfYourself > Maps > L_TestRoom. The project startup
   and default maps still point to the template map; open L_TestRoom explicitly.
3. Open Content > FirstPerson > Blueprints > BP_FirstPersonCharacter.
4. In its Event Graph, find the ghost setup added to Event BeginPlay:
   Get Actor Of Class (BP_RoomController), Set RoomRef, InitializeRoom, and
   Add Mapping Context (IMC_GhostPrototype).
5. Disconnect the execution path into that added ghost-only branch. If a
   Sequence was used, disconnect only the output feeding the ghost setup.
   If it was inserted inline, reconnect the original template execution path.
   Preserve the original BeginPlay behavior and template mapping contexts.
6. Disconnect the Started/Triggered execution wires from IA_Record,
   IA_Restart, and IA_Undo to the old RoomRef function calls. Also disconnect
   any direct R/X/Z key events or character Tick/timers added solely for
   the old recorder. Leave normal Move, Look, and template input connected.
7. Leave unused variables, action assets, ST_GhostSample, BP_Ghost, and
   BP_RoomController assets in Content. They do not need deletion or reparenting.
8. Compile and save BP_FirstPersonCharacter. If compile errors remain,
   inspect the disconnected ghost branch's references; do not delete unrelated
   template logic to fix them.

The native controller polls PlayerController R/X/Z edges. It requires no
IMC_GhostPrototype, RoomRef, or InitializeRoom call. The character still needs
its original Enhanced Input mappings for movement and looking.

## 4. Replace the placed controller

1. Stop Play mode before editing the level.
2. In L_TestRoom's World Outliner, select the placed BP_RoomController.
   Remove its level instance. Keep the Blueprint asset itself.
3. Remove any manually placed old BP_Ghost instances from this test map;
   keep the BP_Ghost asset. The native controller spawns its own ghost.
4. In Content Browser settings, enable Show C++ Classes. Open C++ Classes >
   GhostOfYourself and drag GhostReplayController into the level. Alternatively,
   use Place Actors and search GhostReplayController/Ghost Replay Controller.
5. Confirm exactly one native controller instance is present. Multiple
   controllers would each process the same keys and spawn their own ghost.
6. In that instance's Details, set:

| Property | Value |
| --- | --- |
| Max Recording Seconds | 20 |
| Sample Interval | 0.05 |
| Ghost Class | RecordedGhost (native default) |
| Show Debug Status | Enabled |

7. Leave Ghost Class at RecordedGhost for the first test. The existing BP_Ghost
   is not an ARecordedGhost subclass and cannot simply replace this class.
   The native ghost already supplies a visible cube, movable mesh, no collision,
   no overlap events, no physics, and no independent Tick.
8. Keep the existing First Person GameMode/character. In World Settings,
   ensure the effective GameMode uses BP_FirstPersonCharacter. Do not place
   another possessed pawn if the GameMode already spawns one.
9. Use an unobstructed PlayerStart over the floor. The native controller captures
   the possessed character's actual transform and control rotation once, on
   its first valid Tick. The controller's level position does not set the start.
   Test using normal Play from the default PlayerStart, not Play From Here.
10. Save L_TestRoom. These steps change assets in the editor; they have not
    already been performed by the assistant.

## 5. First five-second test

1. Choose Play in Selected Viewport with one player; do not choose Simulate.
2. Click the game viewport. Expect `C++ ghost ready: R record/save ...` and
   an Exploring status line. If absent, stop and check the native controller
   instance, enabled Tick, and possessed first-person character.
3. Walk away from spawn. Press and release R. You should return to the fixed
   start and original view direction; status should say Recording, Echo 0/1.
4. Walk a short curved route for about five seconds. Turn around a corner,
   then stop at a recognizable endpoint. Press and release R again.
5. You should return to start. Expect a saved-sample message (roughly 100 samples
   for five seconds at normal frame rate). Watch the cube follow the route over
   its recorded duration. It should interpolate instead of jumping to the end.
6. Walk away from the ghost's path to see it clearly. Its cube begins at your
   capsule center, so it may obscure the first-person camera briefly at reset.
7. After replay finishes, wait a few more seconds. The ghost should hold its
   endpoint, and you should be able to walk through it without being blocked.

Record this checkpoint as passed only after observing it in Play mode.

## 6. Recovery and limit checks

| Action | Expected result |
| --- | --- |
| X while replaying, including after the endpoint | Player resets; same ghost starts at time zero. |
| R with Echo 1/1 | Limit message; saved route remains; no second ghost. |
| Z with a saved ghost | Ghost disappears; player resets; Echo 0/1. |
| R, move, X | Working recording cancels; no saved ghost; player resets. |
| R, move, Z while recording | Recording clears and player resets; Echo 0/1. |
| Hold R for a second | Only one key-down action, no repeated toggles. |
| R, then R almost immediately | Safe short recording; no empty-array error. |
| R, move for 20 seconds without saving | One automatic save at 20 seconds; one ghost replays. |
| R after automatic save | Limit message; automatic save remains intact. |
| Z, then R and save a new route | Freed slot accepts the new recording. |

Stop Play, start Play again, and repeat the five-second test once. Recordings
are session-only and are not saved to disk. The cube records actor transforms,
not first-person camera pitch, animation, or button presses. Sampling takes
one actual transform per eligible frame and skips missed intervals after a stall;
interpolation cannot reconstruct movement that occurred during a stalled frame.

## Troubleshooting

- No native class visible: initial module build must succeed; reopen the exact
  nested .uproject after compiling. Live Coding cannot replace that first build.
- R does nothing: click the viewport; confirm Play rather than Simulate and
  a possessed local ACharacter. Native input has no mapping-context setup step.
- Old messages or two ghosts: remove duplicate controllers/old placed ghost
  instances, and revisit the character's old recording execution wires.
- Ghost moves incorrectly: confirm Ghost Class is the native RecordedGhost,
  only one controller exists, and the old Blueprint recorder is disabled.
- Movement/look breaks: restore the original template mapping context and
  template input graph; only the added ghost branch should be disconnected.
- Replays are smooth in the test but fail in PIE: the automation test verifies
  interpolation math, so inspect map/controller/input wiring next.

Do not add puzzle mechanics until the five-second path, 20-second automatic
save, limit preservation, X, and Z checks all pass. Keep existing uncommitted
changes; review Git status before any checkpoint. Commit/push only when requested.
