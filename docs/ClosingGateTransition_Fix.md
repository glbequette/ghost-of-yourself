# Exit transition while a gate starts closing

The packaged diagnostic showed Room 3's gate changing from open to closed
while the player crossed it. The player reached Y=-2456 inside the next room,
but the exit never completed; CurrentRoomNumber remained 3. The old exit guard
required the logical bIsOpen flag, even when the moving door still physically
cleared the player.

GhostGate::AllowsExitForPlayer now accepts an open gate or an unlocked raised
door whose bottom remains above the player's capsule. A fully closed or locked
gate is rejected. The exit still requires the player to cross its box, clear
the doorway, and satisfy the recording/reset/room/checkpoint guards. Successful
completion clears ghosts and updates the fixed recording checkpoint normally.

Save All, close Unreal and run Rebuild-GhostRoomTransition.ps1 from the chat
outputs folder. It builds, tests and opens the project after validation.
Package Windows Shipping again. Walk from Room 3 into Room 4: the counter should
advance to Room 4 and the old ghost should disappear. R or X should then use
Room 4's start marker.

Keep only the intended puzzle responsible for opening each gate. Several lasers
independently writing a gate's open flag can disagree; this change lets an already
passable closing door complete its exit, but does not combine puzzle conditions
or change laser hazards.

UHT and separate MSVC compilation of the gate, exit and regression test passed.
Full automation and packaged verification remain pending. No binary assets
or map hookups were modified.
