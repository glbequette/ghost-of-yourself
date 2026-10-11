# All pressure plates must be occupied

GhostPressurePlateGroup controls one gate using every GhostPressurePlate whose Group Ref points to it. The group has no plate-count cap. Each plate can be held by the live player or any saved ghost. An empty group never opens the gate.

The gate opens only while ALL linked plates are occupied at the same time. Releasing any plate closes it. This is continuous occupancy, rather than the button group's timed presses and latched success.

## Build, validate, and reopen

Save All and close Unreal, then run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\Rebuild-GhostPlateGroups.ps1'
```

The script performs a full rebuild, checks all twelve native tests, and opens the project only after validation passes.

## Link your plates

1. In L_TestRoom, drag GhostPressurePlateGroup into the room from Place Actors or Content Browser > C++ Classes > GhostOfYourself.
2. Set its Room Number to this room's number.
3. Set its Controller Ref to the ONE shared GhostReplayController.
4. Set its Gate Ref to the gate all these plates should open.
5. Select each intended native GhostPressurePlate and set Group Ref to this group.
6. Clear the individual plates' Gate Ref fields. The group owns the gate; grouped plates ignore any leftover individual gate assignments. They also use the group's replay controller, so their individual Controller Ref fields are optional.
7. Remove other plate, beam, or button-group drivers from this same gate. Give each gate one driver.
8. Keep plates separated unless you deliberately want one occupant to hold overlapping activation boxes. A group's membership includes every plate pointing to it, including copied plates elsewhere in the map.

Existing standalone plates work as before: leave Group Ref empty and keep their own Controller Ref/Gate Ref.

## Test and make it solvable

With three plates and two ghosts, record one ghost ending on plate A and another ending on plate B. Stand on C yourself. The gate must open only when all three are occupied. Step off C or remove a ghost with Z: the gate must close.

If the player must leave C to walk through the door, C will be released. For a gate that must stay open throughout the crossing, give the room enough ghost slots to hold EVERY plate and leave the player free to walk through. Set the room's allowance in the replay controller's Room Ghost Limits map. For three separated plates held by ghosts, use three ghost slots.

The group exposes Occupied Plate Count, Total Plate Count, and All Occupied in Details during Play for checking the setup. Plate lights reflect their individual occupancy. The group refreshes all member plates together after replay so gate decisions use current positions. Inactive rooms, reset fades, and locked completed gates cannot open the group gate.

## Validation status

UnrealHeaderTool and scratch MSVC compilation passed for the changed plate, new group, and GhostOfYourself.Puzzles.PlateGroup test. The test covers missing members, three occupants, release, four plates, additional ghosts, inherited controller references, conflicting individual gate assignments, room/reset guards, locked gates, and empty groups. Full linking and execution of all twelve tests remain pending the PowerShell script. No binary map or asset files were directly edited; you must link the actors in the editor.

