# Working title screen on game launch

The native menu uses your white wispy logo and lab background, with real BEGIN/QUIT controls. BEGIN fades directly into L_TestRoom (Room 1). L_Intro is deferred for now. Mouse click works on both buttons; Tab or Up/Down changes keyboard selection, and Enter/Space activates it. The menu has its own GameMode with no playable pawn, so the player and narrator do not start behind it.

## 1. Rebuild with Unreal closed

Save All and close Unreal. Run in normal PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\glbeq\Documents\Codex\2026-10-10\continue-development-of-my-36-hour\outputs\Rebuild-GhostTitle.ps1'
```

Wait for **Build succeeded and all six native tests passed**. If it fails, leave Unreal closed and share the first error. This runs Recording.Playback, Puzzles.PlateGate, Puzzles.ExitZone, Puzzles.Buttons, Intro.Flow and Title.Flow. Do not use Live Coding.

## 2. Run the setup inside Unreal

Reopen the project, stop Play, and **Save All**. Ensure Python Editor Script Plugin and Editor Scripting Utilities are enabled. In Output Log's normal console command input run:

```text
py "C:/Users/glbeq/Documents/Codex/2026-10-10/continue-development-of-my-36-hour/outputs/TitleScreen/setup_title_screen.py"
```

For future checkouts, the same script and source art are also saved in the repository under SourceArt/TitleScreen, with the intro kit under SourceArt/IntroRoom.

The script:

- Uses the existing L_TestRoom as the BEGIN destination; no intro level or narration import is required.
- Imports the independent logo/background as UI textures into `/Game/GhostOfYourself/UI/Title`.
- Creates an empty `/Game/GhostOfYourself/Maps/L_Title`, using native **GhostTitleGameMode**. No Widget Blueprint or Level Blueprint wiring is needed.
- Sets **Game Default Map** to L_Title, preserving your Editor Startup Map and gameplay GameMode.
- Adds L_Title and L_TestRoom to packaging map settings, and marks the title-texture folder to cook.
- Backs up the original DefaultEngine.ini and DefaultGame.ini next to them with `.before-title-menu.bak` suffixes before changing settings.

Existing artwork is reused. Existing maps are preserved. If an existing L_Title has a different GameMode, setup stops and asks you to assign GhostTitleGameMode in that map's World Settings, save, then rerun. Startup configuration changes happen only after all required levels/assets have been created successfully. If a runtime cache update reports an error after those writes, restart the editor and inspect Maps & Modes/Packaging before testing.

## 3. Test the menu

The script opens L_Title. Press Play; if you are testing later from another map, open L_Title first. PIE plays the current editor map, while Game Default Map controls a normal launch without an explicit map.

1. Confirm the logo has clean transparent edges and the lab background fills the view. Both buttons should be visible.
2. Move the pointer over BEGIN and QUIT. Check hover feedback and that each clickable area matches its button.
3. Use Tab or Up/Down, then Enter/Space to test keyboard selection.
4. Click BEGIN. It should fade out, hide the cursor and load L_TestRoom. Double-clicking should still request only one transition.
5. Room 1 should start directly. Confirm you can look and walk normally and no intro narration plays.
6. Check the custom player, Room 1 recording/plate/gate and Room 2 synchronized buttons still work.
7. Test QUIT in Standalone or a packaged build. In PIE, quitting may end the play session rather than closing the editor.
8. Resize the game window and check both controls remain visible and clickable. Background cropping preserves its aspect ratio; logo/button layout scales to fit.

If there is a menu with plain fallback text or no background, check the imported textures at these exact paths:

```text
/Game/GhostOfYourself/UI/Title/T_GhostTitleLogo
/Game/GhostOfYourself/UI/Title/T_GhostTitleBackground
```

If BEGIN reports a missing starting level, check `/Game/GhostOfYourself/Maps/L_TestRoom` and the setup log. A missing destination leaves the menu usable rather than travelling to an invalid level.

## 4. Verify game launch

Check **Project Settings > Maps & Modes > Game Default Map = L_Title**. Editor Startup Map can remain your working map; changing it is optional.

Check **Packaging > List of maps to include** contains L_Title and L_TestRoom, and **Additional Asset Directories to Cook** contains `/Game/GhostOfYourself/UI/Title`. Restart the editor before packaging if packaging settings were already open while setup ran, then recheck them.

Launch a fresh packaged game without a map argument: it should start at the title menu, then progress title → Room 1. A working PIE test alone does not verify a packaged launch. The source-art PNG/WAV files are import sources; the saved Unreal textures/audio/maps are the runtime assets that must be cooked.

## Code and verification

Added GhostTitleScreen.h/.cpp and Tests/GhostTitleTests.cpp. The menu is drawn by a native HUD; it requires no new UMG dependency. The title controller defaults to L_TestRoom. Its property is displayed as Begin Destination; it can be changed to L_Intro later when that scene is ready.

UnrealHeaderTool passed. The title implementation, title test and updated intro controller compile to scratch object files. The config helper passed checks for preserving unrelated settings and the editor map, startup-map replacement, backup creation and duplicate-free repeated cooking entries. PowerShell/Python syntax passed. Full linking, six engine automation tests, editor setup, mouse/keyboard checks and packaged launch remain pending. Existing binary maps/assets were not directly edited by Codex. No commit or push was made for this implementation.

