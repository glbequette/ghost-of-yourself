# Per-room ghost limits and button groups

## Build, validate, and reopen

Save All and close Unreal, then run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\Rebuild-GhostRoomSettings.ps1'
```

The script performs a full rebuild, verifies all eleven native automation tests, and opens the project only after validation succeeds.

## Set each room's ghost allowance

Select the ONE shared GhostReplayController. In Details, expand Ghost > Multiple Ghosts.

- Default Ghost Limit: allowance for rooms with no explicit entry. Initially 1.
- Room Ghost Limits: a map from room number to the number of saved ghosts allowed there.

Click + in Room Ghost Limits. Set the integer key to the room number and the value to its ghost allowance. For example:

| Room key | Ghost limit value |
| --- | --- |
| 6 | 2 |
| 7 | 3 |
| 8 | 1 |
| 9 | 4 |

Room 6 is initially seeded with 2 to preserve the current puzzle. There is no longer a Room 6-and-later threshold. If Room 7 has no entry, it uses the default allowance of 1.

A value of 0 disables saving ghosts in that room. Negative values are treated as 0. There is no artificial three-ghost cap. Room 0 remains the existing non-recording welcome room. The setting counts saved ghosts, excluding the live player. Normally N separated simultaneous buttons call for N−1 ghosts.

Set limits before starting Play, then save the map. R can record even at the room limit. Saving replaces the oldest ghost; cancelling keeps existing recordings. If you lower a limit during Play, a successful save retires enough oldest ghosts to fit the new allowance. Z removes the newest ghost, X restarts the shared clock, and room transitions or developer jumps clear every ghost. The HUD displays the current room's count and limit.

The first three ghosts remain blue, yellow, and green. Additional slots use further hues and retain their actual slot numbers. Recording, endpoint holding, plate activation, beam shielding, and button replay work across all saved tracks.

## Any number of buttons in a group

Select the room's GhostButtonGroup and assign its Room Number, shared Controller Ref, and exit Gate Ref. Set each intended GhostButton's Group Ref to this same group.

The group automatically counts every linked button. It has no maximum member count and no separate Required Buttons setting. All members must be pressed within Sync Window Seconds before the gate unlocks. Each button can be pressed only once per attempt. Repeats are rejected, even after the sync window expires. X restarts the attempt to retry. A single-button group works; an empty group stays closed.

Keep each gate's driver unique. Ensure copied buttons in another room do not accidentally reference this group, since they would also become required members. If the buttons do not accept E, verify the HUD room number, the group's Room Number, its Controller Ref/Gate Ref, button Group Ref, and the player's range and clear view of the panel.

## Validation status

UnrealHeaderTool and scratch MSVC compilation passed for the changed controller, group, ghost appearance code, and automation tests. Tests now cover explicit room allowances, the default fallback, disabled recording slots, four ghosts, and a twelve-button group with timing and distinct-member requirements. Full module linking and execution of all eleven tests remain pending the PowerShell script. No binary assets or maps were edited directly; assigning limits and actor references is editor work for you.



