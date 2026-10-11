# Room 3: Ghost-blocked security beam

The native actor is GhostSecurityBeam. It checks the ghost's position without enabling collision or physics. A ghost anywhere along the beam shields only the section beyond itself and opens its optional linked gate. The red emissive cylinder stays visible from the emitter to the first contact with the ghost's virtual capsule; the section beyond the ghost disappears. The visible section between the emitter and the ghost stays dangerous: touching or crossing it resets the player. Cross on the far side of the ghost. A small pulsing impact sphere appears at the contact point, using the beam material; adjust Impact Glow Radius (default 6 cm) for its size. When the ghost moves away, the beam returns and the gate closes. The saved ghost holds its final recorded position, so ending the recording inside the beam keeps it interrupted.

An active beam restarts the player's attempt using the existing fade and checkpoint, preserving the saved ghost. Recording is harmless so the player can demonstrate where the ghost should stand. Beams only operate in their assigned Room Number and are harmless during reset fades. Motion checks include crossings between frames; the swept check uses conservative capsule-expanded bounds near the beam's ends.

## 1. Build with Unreal closed

Save All, close Unreal, then run in PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\Rebuild-GhostBeam.ps1'
```

This performs a full rebuild and runs all eight native automation tests, including GhostOfYourself.Puzzles.SecurityBeam. It opens the Unreal project automatically only after every validation passes. Do not use Live Coding for this change.

## 2. Place the beam in Room 3

1. Reopen L_TestRoom. Keep the ONE existing shared GhostReplayController.
2. Find GhostSecurityBeam in Place Actors, or Content Browser > C++ Classes > GhostOfYourself, and drag it into Room 3.
3. Put its origin on one side of the passage, about 100 cm above the floor. Keep actor scale at 1,1,1.
4. Its beam runs along the actor's local +X axis. Rotate the actor in yaw to aim it across the passage.
5. Select its BeamEnd component at the top of Details. Set its relative Location X to the passage width, 1200 cm for a 12-metre passage; Y and Z remain 0 for a horizontal beam. New actors default to 1200 cm. For an actor already placed in the map, manually change this value after rebuilding; saved component overrides are preserved. The cylinder updates to connect the ends.
6. Set Controller Ref to the shared replay controller. Set Room Number to 3.
7. Leave Beam Radius at 2, Ghost Radius at 34, and Ghost Half Height at 88 initially. These virtual ghost dimensions should match the player's capsule, not the skeletal mesh bounds.

Use walls at both beam ends so the player cannot walk around it. A low passage ceiling around 220 cm prevents jumping over a beam at 100 cm. Keep the Room 3 checkpoint comfortably before the beam, at least 150 cm away. A checkpoint inside the beam would repeatedly restart the player.

## 3. Give the beam its red material

In Unreal's Output Log, run:

```text
py "C:/Users/glbeq/Documents/Codex/2026-10-10/continue-development-of-my-36-hour/outputs/create_security_beam_material.py"
```

Select GhostSecurityBeam > BeamMesh > Materials > Element 0 and assign M_SecurityBeam. The actor uses a thin engine cylinder; the script supplies its red emissive appearance. Without the material it still works but looks like a plain cylinder. The script preserves an existing material of the same name.

## 4. Link Room 3's exit gate

1. Place or reuse a GhostGate at the Room 3 exit. Set Destination Room Number to 4.
2. Assign that gate to the beam's Gate Ref. Give this gate only ONE driver: this beam. Remove any plate/button-group connection that also controls it.
3. Place a GhostExitZone beyond the gate on the Room 4 side. Assign its Controller Ref to the shared replay controller, Gate Ref to this gate, and From Room Number to 3.
4. Add a floor-level TargetPoint inside Room 4, safely beyond the doorway. Assign it to the zone's Next Room Start.
5. Room 2's exit zone must have From Room Number 2 and a Next Room Start TargetPoint on the safe side of Room 3's beam. Entering Room 3 must change the HUD room number to 3 before testing this mechanic.

Crossing the exit clears the ghost and adopts Room 4's checkpoint. The exit zone locks the gate closed; the beam actor continues moving the gate closed after the room transition.

## 5. Test one beam first

1. Enter Room 3 normally. Walk into the beam without recording: it should fade and return you to the Room 3 checkpoint.
2. Press R, walk into the beam, and stop. Recording must continue safely.
3. Press R again to save. You return to the checkpoint while the ghost replays the route.
4. Wait until the ghost enters the beam: the cylinder stops at the ghost and the exit gate rises. The ghost holds its endpoint.
5. Cross the passage on the FAR side of the ghost and enter the exit zone: the room number advances to 4, the ghost clears, and the gate closes behind you.
6. On another attempt, save a recording that walks through the beam and stops outside it. The beam should return to its full 12-metre length when the ghost leaves it.
7. While the ghost is blocking, cross BETWEEN the emitter and ghost: you must reset, keeping your saved ghost. The impact glow must disappear when the ghost stops blocking.
8. Z removes the saved ghost and resets the attempt. X restarts playback; both must restore the beam according to the ghost's new position.

Tutorial text and narration are left for you to author. Suggested concepts: recording is safe, ghosts interrupt beams, and finishing a recording while standing in the beam holds it disabled.

## Validation status

UnrealHeaderTool and scratch MSVC compilation of the new actor and test passed. The full game module has not been linked and the new automation test has not been executed by the assistant. The PowerShell script performs those remaining checks. Editor placement and material assignment still need to be done by you; no binary map or asset files were edited directly.



