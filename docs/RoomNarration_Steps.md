# Imported room voice-over

The shared GhostReplayController now contains a RoomNarration component with these native default playlists:

| Room | Clips, in playback order |
| --- | --- |
| 0 | GOY_IntroRoom → GOY_RecordInfo → GOY_CancelInfo |
| 1 | GOY_PressurePlate |
| 2 | GOY_Buttons |
| 3 | GOY_Lasers |
| 6 | GOY_MultipleGhosts |

The seven imported assets were found under /Game/GhostOfYourself/Audio. The default references use those exact assets. Rooms without playlists are silent.

Room entry starts narration after 0.75 seconds. Clips in a playlist play sequentially, with a 0.35-second gap. Narration is non-spatial so it stays audible while moving around the lab. Recording and attempt resets do not restart the tutorials. Moving into another room stops the previous room's audio before selecting the new playlist. A developer room jump deliberately replays the destination room's tutorial.

## Build, validate, and reopen

Save All, including your imported audio assets, and close Unreal. Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\Rebuild-GhostNarration.ps1'
```

This performs a full rebuild, validates all fourteen native tests, and opens the project only after validation succeeds.

## Room 0 hookup

Keep your existing GhostIntroController and shared replay controller. On the intro actor, confirm Replay Controller Ref points to the shared controller and Use Room Narration is enabled (the new default).

The intro's previous single Introduction Audio value is ignored in this mode. It does not play over the new sequence. The welcome gate stays closed until all three Room 0 clips finish. Enter skips the entire sequence and opens the gate. Existing Gate Ref, Room One Start, and ExitBox setup remain as before.

If Use Room Narration is disabled, or the intro has no Replay Controller Ref, the old single-clip/separate-level behavior remains available.

## Inspect or change clips

Select the shared replay controller and select its RoomNarration component at the top of Details. Expand Ghost > Narration > Room Playlists. The map key is the room number, and each value contains an ordered Clips array. The defaults above should already be assigned; no extra room actors or trigger boxes are needed.

You can change the clip arrays, Room Start Delay, Clip Gap, and the audio component's Volume Multiplier in the editor. Keep imported clips non-looping. If your placed component retains different playlist values, assign the clips according to the table and save the map. The native default references ensure the audio is part of the actor's referenced assets; also test audibility in your packaged build before publishing.

## Check playback

Start in Room 0 and listen for the three clips in order. Check that the gate stays closed until the third finishes, or Enter skips all three. Enter Rooms 1, 2, 3, and 6 to hear their assigned clips. Press R or X during a tutorial: it should continue, rather than restart. Use a developer number shortcut to revisit a room and replay its tutorial.

## Validation status

UnrealHeaderTool and scratch MSVC compilation passed for the narration component, controller, intro integration, and GhostOfYourself.Audio.RoomNarration test. The test validates the imported assets and playlist order, Room 0 gate timing, room switching, reset behavior, developer replay, and skipping. Full linking, execution of all fourteen tests, audible playback, and packaged-build verification remain pending. No binary assets or maps were directly edited by the assistant.
