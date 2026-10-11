# Room 10 completion

GOY_Completion is already imported under /Game/GhostOfYourself/Audio.
The native controller now treats crossing Room 10's exit as finishing the game.
It clears all ghosts, stops player movement, plays GOY_Completion once, waits for
it to finish, fades the picture and audio to black, then opens L_Title.
The music stays quiet underneath completion narration.

1. Save All and close Unreal. Run Rebuild-GhostCompletion.ps1 in this outputs
   folder. It builds, imports the lab music if needed, checks all sixteen tests,
   and opens Unreal only after successful validation. Keep the Music folder
   beside the script.
2. In L_TestRoom, select the GhostExitZone after the final gate.
3. Set From Room Number to 10. The actor label GhostExitZone10 alone does not
   select Room 10. Set Controller Ref to your single shared GhostReplayController.
   Gate Ref is optional for this final exit.
4. Put ExitBox beyond the gate, where the player's whole capsule can clear the
   doorway. The player entering this final box completes the game regardless
   of gate state or doorway alignment. Leave Next Room Start empty. No Room 11 needs
   to be built; that number is used internally while completion plays.
5. The replay controller's RoomNarration component should show a playlist entry
   for 11 with GOY_Completion. If your placed actor has an overridden playlist,
   add that entry manually. Keep the clip's Looping option off.
6. Save All. Play, use F10 to test Room 10 if its cheat checkpoint is configured,
   solve its puzzle, and walk through the final gate. Verify the completion clip
   finishes before the title screen appears. Start again from the title screen
   and verify normal gameplay is restored.

The replay controller defaults Completion Title Level to
/Game/GhostOfYourself/Maps/L_Title. Completion Return Delay defaults to 0.75
seconds after the clip ends, including a 0.5-second fade. R/X/Z/E and developer
room jumps are blocked during completion; looking around remains available
until the fade starts. The normal tutorial clips and music use their own clocks.
Muted or unavailable audio uses the existing narration-duration fallback so the
game still returns to the title. Missing clips likewise do not trap the player.

Code checks completed here: Unreal Header Tool and separate MSVC compilation
of the changed controller, narration, exit zone, and completion test. Full editor
linking and test execution are pending the script. The map setup and listening
test must be done in Unreal; no binary map or asset files were edited here.
