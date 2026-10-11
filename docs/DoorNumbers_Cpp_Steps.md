# Destination room numbers on doors

GhostGate now has two small TextRender components attached to the moving door panel, one facing each side. They display only a two-digit destination such as `02`, without a prefix. The room number identifies the room beyond that door. Both faces show the same identifier. Text has no collision and does not change the gate's movement, lock or puzzle state.

## Build

Save All and close Unreal, then run in normal PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\Rebuild-GhostDoors.ps1'
```

Wait for the build and all six tests to pass. Do not use Live Coding for the new components. If it fails, leave Unreal closed and share the first error.

## Set the numbers

1. Reopen L_TestRoom.
2. Select the native **GhostGate** actor that leaves Room 1, rather than its decorative static door frame.
3. In Details, search **Destination Room Number** and set **2**. Its panel should show **02** in the editor and Play.
4. Select Room 2's exit GhostGate and set **3**, so it shows **03**.
5. Set **0** on internal gates or any door that should have no number. For a future intro-to-Room-1 gate, use **1**.
6. Save the level. When copying gates into another room, update the copied Destination Room Number alongside its puzzle/exit references.

Under **Ghost > Door Number**, **Room Number Size** defaults to 26 cm and **Room Number Color** defaults to dark graphite for the ivory lab doors. Use a lighter color if you switch to dark door panels.

The labels are placed above the panel center using the static mesh bounds, 2 cm beyond its front/back faces. They follow panel position and rotation while keeping text scale independent of the thin door-mesh scale. Use the native gate's usual local Y thickness and Z height orientation; custom meshes oriented along other axes need different placement.

## Play checks

Confirm both sides are readable when closed; rotate the gate actor and verify the text rotates with it. Open it with the existing plate or synchronized buttons: the number must rise with the door. Restart to lower it, and complete the room to confirm the one-way lock still works. The number should not change recording, collision or exit behavior.

## Verification

UnrealHeaderTool passed; GhostGate.cpp and the updated GhostPressurePlateTests.cpp compiled to scratch objects. The plate/gate test now checks the destination text, both faces, label movement with the panel, nonblocking labels and clearing the number with 0. Full linking/automation execution and visual editor checks are pending the build above. Existing maps/assets were not directly edited; you must set the per-door numbers in the editor. No commit/push was made.

