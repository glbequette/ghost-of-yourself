# Title responsiveness update

Save All, close Unreal, and run Rebuild-GhostTitleResponsive.ps1 from this folder.
The script builds, validates all sixteen native tests, and reopens the project.
The Music folder must remain beside the script for its music import step.

Open L_Title and press Play. Click Begin once; its label should immediately
change to LOADING before the fade. Quit responds to a single click as well.
In Play In Editor, the editor may require the initial click to focus its Play
viewport before it sends keys or mouse input to the game. Standalone Game is
the better way to check how this feels at launch.

The menu is click-only. Enter, Space, navigation keys, and gamepad confirmation
do not activate buttons. Mouse clicks use the drawn button bounds directly.
Hover changes color. There is no menu-navigation instruction text.

Try a small window and a fullscreen window. Buttons retain at least 260x48
pixels in the supported tested viewport sizes, with a separate gap, centered
labels. Logo height is constrained to keep it
above the buttons. Existing logo/background textures are reused.

No map or texture files were edited. L_Title must still use GhostTitleGameMode
in World Settings > GameMode Override. Full linking and automation execution
are pending the rebuild script. Unreal Header Tool and separate MSVC compilation
of the title controller/HUD and updated test passed here.
