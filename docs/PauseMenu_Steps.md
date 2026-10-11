# Pause and settings

Save All and close Unreal. Run Rebuild-GhostPause.ps1 from this outputs folder.
It builds, checks all seventeen native tests and opens the project after
validation. Keep the Music folder beside the script.

The shared GhostReplayController automatically owns a PauseMenu component.
No extra actor, Blueprint wiring or binary asset edits are needed.

During gameplay, Esc opens the pause menu. Continue or Esc resumes the same
attempt. Settings opens four sliders and a Back button. Quit exits the game;
in Play In Editor it stops that Play session. The menu uses standard Slate
buttons and sliders. Esc is handled before editor Play shortcuts while the
game viewport has focus, including when the settings page is open.

Mouse sensitivity ranges from 0.10x to 3.00x; the default is 1.00x. It modifies
the template's IA_MouseLook action at runtime without changing the saved asset.
Music, narrator and master volume range from 0% to 100%. Music remains quiet
during narration, with the music slider multiplying that existing mix. Narrator
volume applies to all tutorial and completion clips. Master volume scales the
game's entire audio device, including sound effects. Changes apply immediately.
Settings save on releasing a slider, clicking Back, resuming, or quitting, and
reload from Saved/Config/WindowsEditor/GameUserSettings.ini in the editor (the
platform's GameUserSettings.ini in packaged builds).

The number keys, numpad and F10 no longer jump rooms. Old placed-actor cheat
properties cannot re-enable these shortcuts. The compatibility JumpToRoom
method always returns false. Old room-cheat guides are superseded by this change.

Play-test checklist:
- Open the menu while recording. Wait, Continue, and check that the recording
  continues at the same time rather than gaining the paused seconds.
- Pause while ghosts replay and while narration plays. Resume and check both
  continue from the pause point.
- Change sensitivity, then Continue and look around. Compare 0.5x and 2.0x.
- Mute music alone, narration alone and then master volume. Verify each control
  affects the expected sound. Restore the desired volumes.
- Restart the game and check that the chosen settings remain.
- Press numbers and F10: the current room must not change.
- Test Quit in Standalone and test the existing title/completion flow.

Pause is unavailable during the final completion sequence so it can finish
returning to the title. Existing controls, room counters and artwork are preserved.

UHT and separate MSVC compilation passed for the changed code and tests. Full
linking, automation execution, real clicks, audio and persistence checks remain
pending the rebuild and your Play test.
