# One ghost: editor implementation checkpoints

> **Superseded by [OneGhost_Cpp_Steps.md](OneGhost_Cpp_Steps.md).**
> Keep these notes only as a record of the old prototype. Follow the C++ guide
> to disconnect the old recording setup and place the native controller.

Scope: L_TestRoom, one saved ghost, 20-second maximum, R to record/save,
X to cancel/restart, Z to remove the ghost. No plates or button events yet.
These are instructions to implement in Unreal, not an implemented Blueprint.

## What was inspected

- Project: `GhostOfYourself/GhostOfYourself.uproject`; engine association `5.8`.
  That association does not identify the installed patch version.
- Input configuration uses Enhanced Input.
- Template character: `Content/FirstPerson/Blueprints/BP_FirstPersonCharacter`.
- Custom assets: BP_Ghost, BP_RoomController, ST_GhostSample, L_TestRoom.
- The struct's asset metadata contains Time and Transform. Confirm their types
  in the editor; binary metadata inspection does not verify graph behavior.
- Startup/default maps still point to `/Game/FirstPerson/Lvl_FirstPerson`.
- Custom content and map external actor/object folders were untracked at inspection.
- No AGENTS.md was found in the searched project or checked ancestor paths.

The supplied plan is design context. This guide follows the user's narrower request
and existing nested layout; it does not repeat its project setup or build ten rooms.

## 1. Open the test map and check the existing data

1. Open the nested .uproject. Open `Content/GhostOfYourself/Maps/L_TestRoom`.
2. Confirm exactly one BP_RoomController is placed in this map.
3. Open ST_GhostSample. Confirm `Time` is Float and `Transform` is Transform.
   If these already exist with these types, leave them alone. Save.
4. Open BP_Ghost. Keep a Scene Component as its root and the cube as a child.
   Set cube Mobility to Movable, Collision Presets to NoCollision,
   Generate Overlap Events off, and Simulate Physics off.
5. Keep actor/root scale at 1,1,1. Resize the child cube, if desired.
   The ghost's actor origin will follow the character's capsule center.
6. In BP_Ghost Class Defaults, disable actor tick. The room controller will drive it.

Checkpoint: both Blueprints compile. Do not add ghost input or ghost physics.

## 2. Add room-controller variables

In BP_RoomController > My Blueprint > Variables, create each variable, select its
type in Details, then Compile to expose its default value. For arrays, use the
container selector beside the type and choose Array.

| Name | Type | Default |
| --- | --- | --- |
| PlayerRef | Character Object Reference | None |
| StartTransform | Transform | captured at initialization |
| StartControlRotation | Rotator | captured at initialization |
| bInitialized | Boolean | false |
| bRecording | Boolean | false |
| Clock | Float | 0 |
| MaxRecordingSeconds | Float | 20 |
| SampleInterval | Float | 0.05 |
| LastSampleTime | Float | 0 |
| WorkingSamples | ST_GhostSample Array | empty |
| SavedSamples | ST_GhostSample Array | empty |
| GhostRef | BP_Ghost Object Reference | None |

For MaxRecordingSeconds, enable Instance Editable. One saved array enforces the
one-ghost limit in this milestone. Multi-ghost data comes later.

## 3. Initialize from the player and add input

Create a BP_RoomController function `InitializeRoom` with input `Player`
(Character Object Reference). Its execution chain:

1. Branch on bInitialized. True: return. False: continue.
2. Set PlayerRef = Player.
3. Set StartTransform = PlayerRef -> Get Actor Transform.
4. Set StartControlRotation = PlayerRef -> Get Controller -> Get Control Rotation.
5. Set bInitialized = true. Print String `Ghost controller ready`.

In `Content/GhostOfYourself/Blueprints`, create Input Action assets IA_Record,
IA_Restart, and IA_Undo. Set each Value Type to Digital (bool). Create an Input
Mapping Context IMC_GhostPrototype. Add R -> IA_Record, X -> IA_Restart,
and Z -> IA_Undo. Leave action triggers/modifiers empty for this test.

Open BP_FirstPersonCharacter:

1. Add `RoomRef`, type BP_RoomController Object Reference.
2. Extend its existing Event BeginPlay execution chain; preserve existing logic.
   Use a Sequence if needed. Do not add a second BeginPlay event.
3. Get Actor Of Class (BP_RoomController) -> Set RoomRef -> Is Valid.
4. Valid: call InitializeRoom on RoomRef, with Self as Player.
5. Get Player Controller (0) -> Get Enhanced Input Local Player Subsystem ->
   Add Mapping Context, context IMC_GhostPrototype, priority 1.
   Validate the subsystem before using it.
6. Invalid RoomRef: Print String `No room controller in this map`.

Add Enhanced Input event nodes for the three actions. Use **Started**, which
invokes one command on key down. On each Started execution, validate RoomRef,
then call the appropriate function created below. Do not use Triggered for toggles.

Checkpoint: Play L_TestRoom and confirm `Ghost controller ready` prints once.
Click the game viewport to give it keyboard focus. Stop Play before editing again.

## 4. Reset the player and capture a sample

Create BP_RoomController function `ResetAttempt`:

1. Set bRecording = false, then Clock = 0.
2. PlayerRef -> Get Character Movement -> Stop Movement Immediately.
3. Break Transform(StartTransform). Set Actor Location And Rotation on PlayerRef:
   New Location = saved location; New Rotation = saved rotation;
   Sweep = false; Teleport = true.
4. PlayerRef -> Get Controller -> Set Control Rotation(StartControlRotation).
5. If GhostRef is valid, call its EvaluateAtTime(0), added in checkpoint 6.

Use the captured character transform, not a PlayerStart's floor-level location:
the character actor origin is at capsule center. This milestone captures one fixed
start per Play session. Later rooms can supply their own explicit start transform.

Create `CaptureSample`:

1. Make ST_GhostSample: Time = Clock, Transform = PlayerRef -> Get Actor Transform.
2. Add it to WorkingSamples.
3. Set LastSampleTime = Clock.

This records world movement and actor facing. First-person camera pitch is not
part of the cube prototype. Camera/head facing can be added separately later.

## 5. Start, save, and tick recording

Create function `ToggleRecording`, called by IA_Record Started:

1. Branch bInitialized. False: return.
2. Branch bRecording. True: call CommitRecording and return.
3. Otherwise, Branch (SavedSamples Length > 0).
   True: Print String `Echo limit reached - Z to remove it`; return.
4. False: ResetAttempt -> Clear WorkingSamples -> CaptureSample ->
   Set bRecording = true -> Print String `Recording - R to save, X to cancel`.

Use BP_RoomController Event Tick (enable actor tick in Class Defaults):

1. Branch bInitialized. False: do nothing.
2. Branch bRecording.
3. Recording branch: Set Clock = Min(Clock + Delta Seconds, MaxRecordingSeconds).
4. Branch (Clock >= MaxRecordingSeconds).
   True: CommitRecording.
   False: if Clock - LastSampleTime >= SampleInterval, call CaptureSample.
5. Non-recording branch: if GhostRef is valid, Set Clock = Clock + Delta Seconds,
   then GhostRef -> EvaluateAtTime(Clock).

Sampling is approximately 20 Hz, once per eligible frame. Actual timestamps
allow interpolation when frame intervals vary. Do not fabricate multiple samples
with the same current position to catch up after a slow frame. This first prototype
does not recover movement detail that occurs entirely within a stalled frame.

Create `CommitRecording`:

1. Branch bRecording. False: return.
2. If Clock <= 0, Print String `Record a little movement first`; return.
3. CaptureSample to include the exact endpoint. Equal timestamp duplicates are
   safe because EvaluateAtTime below guards a zero-length interval.
4. Set bRecording = false.
5. Set SavedSamples = WorkingSamples (array value copy). Clear WorkingSamples.
6. Spawn Actor From Class BP_Ghost. Spawn Transform = SavedSamples[0].Transform.
   Collision Handling Override = Always Spawn, Ignore Collisions.
7. Set GhostRef = spawn Return Value. Validate GhostRef.
8. Valid: set GhostRef.Samples = SavedSamples, then ResetAttempt.
   Print String `Saved - echo replaying`.
9. Spawn failure: Clear SavedSamples -> ResetAttempt -> Print String
   `Ghost spawn failed - try recording again`.

Define the BP_Ghost Samples variable and EvaluateAtTime before compiling callers.
Blueprint references between these checkpoints will be incomplete until then.

## 6. Make the ghost interpolate on the room's clock

In BP_Ghost create `Samples` (ST_GhostSample Array) and `PlaybackIndex`
(Integer, default 0). Create function `EvaluateAtTime` with input `Time` (Float).
Give it local variables N (Integer), A/B (ST_GhostSample), Duration and Alpha (Float).

1. Set N = Samples Length. If N == 0, return without indexing the array.
2. If N == 1: Set Actor Transform to Samples[0].Transform and return.
3. If Time <= Samples[0].Time: Set PlaybackIndex = 0;
   Set Actor Transform to Samples[0].Transform; return.
4. If Time >= Samples[N-1].Time: Set Actor Transform to
   Samples[N-1].Transform; return. This holds the endpoint without looping.
5. If Time < Samples[PlaybackIndex].Time, set PlaybackIndex = 0.
6. Add a WhileLoop. Condition:
   `(PlaybackIndex < N-2) AND (Time >= Samples[PlaybackIndex+1].Time)`.
   Loop Body: Set PlaybackIndex = PlaybackIndex + 1.
   Its index range is bounded: both array accesses are valid when N >= 2.
7. From Completed: Set A = Samples[PlaybackIndex];
   Set B = Samples[PlaybackIndex+1].
8. Set Duration = B.Time - A.Time.
9. Branch Duration > 0.000001.
   True: Set Alpha = Clamp((Time - A.Time) / Duration, 0, 1).
   False: Set Alpha = 1. Both execution paths join the next operation.
10. Lerp (Transform): A = A.Transform, B = B.Transform, Alpha = Alpha.
11. Set Actor Transform on Self to the Lerp result: Sweep false, Teleport true.

All Set Actor Transform nodes here use Sweep false and Teleport true. Ghost
collision and physics stay disabled. No ghost Event Tick, timer, or independent
clock is needed. ResetAttempt evaluates time zero, resetting PlaybackIndex.

Checkpoint: Compile BP_Ghost, then BP_RoomController, then the character.
Play, press R, walk a curved route for five seconds, press R again. You should
return to the start while the ghost repeats the route and stops at its endpoint.

## 7. Cancel, restart, and remove

Create BP_RoomController `CancelOrRestart`, called by IA_Restart Started:

1. If not initialized, return.
2. Set bRecording = false; Clear WorkingSamples; ResetAttempt.
3. Print `Restarted` if SavedSamples Length > 0, otherwise `Exploring`.

SavedSamples is deliberately preserved. X during recording creates no ghost;
X during replay returns you and the ghost to the fixed start.

Create `UndoGhost`, called by IA_Undo Started:

1. If not initialized, return.
2. Set bRecording = false; Clear WorkingSamples.
3. If GhostRef is valid, Destroy Actor(GhostRef).
4. Set GhostRef to None; Clear SavedSamples; ResetAttempt.
5. Print String `Echo removed - R to record`.

With one slot, Z clears the saved ghost. Full reset, a hold-to-confirm control,
room transitions, and room-state resets are later milestones.

## 8. Verify in Play before the checkpoint commit

- R starts from the same location and view direction after you explore elsewhere.
- Holding R does not repeatedly start/commit.
- R saves a short path, resets the player, and starts one ghost from time zero.
- The ghost follows the path, turns, never blocks you, and holds its endpoint.
- X during replay restarts the path from the beginning, including after it ended.
- R with a saved ghost prints the limit message without changing that recording.
- Z removes it and permits a new recording.
- X during recording cancels without creating a ghost.
- A 20-second recording commits automatically exactly once.
- An almost immediate save does not access an empty array or divide by zero.

If R does nothing: check viewport focus, character possession, the added mapping
context, Started wiring, and RoomRef validity. If the ghost does not move: check
its movable cube, Samples assignment, and the controller's enabled tick.

Save All in Unreal. In PowerShell at the repository root, review `git status --short`
and `git lfs ls-files` before staging. Include the map's external actor/object
folders with the map and Blueprints. The repository ignore patterns already cover
nested generated directories. Commit only after the manual checks work.

## Responsibility and reference

You perform the editor changes above. The assistant can edit Markdown, configuration,
and source files; this guide does not modify .uasset/.umap files or claim a tested
playable result. No project configuration change is required for this milestone.

Epic references:
- [Enhanced Input](https://dev.epicgames.com/documentation/unreal-engine/enhanced-input-in-unreal-engine)
- [Set Actor Location And Rotation](https://dev.epicgames.com/documentation/unreal-engine/BlueprintAPI/Transformation/SetActorLocationAndRotation)

After this works, add one pressure plate and one gate in L_TestRoom. Recorded
button attempts and multi-ghost storage are separate subsequent milestones.
