# Standard title buttons

Save All and close Unreal, then run Rebuild-GhostTitleButtons.ps1 from this
outputs folder. It builds, checks all sixteen native tests and opens the project
after validation. Keep the Music folder beside the script.

Open L_Title and Play. The menu now uses actual Slate SButton widgets. Clicking
Begin or Quit triggers the widget's own OnClicked event; no manually calculated
canvas hit boxes or PlayerController mouse-coordinate checks are involved.
Buttons are not keyboard focusable, so Enter/Space do not confirm actions.
There is no navigation text. The existing HUD still draws the logo/background
and the fade. Button widgets are removed when leaving the title map and recreated
when returning after completion.

No editor wiring or map changes are required. The log confirms L_Title already
uses GhostTitleGameMode. Test both Begin from the title and the return to the
title after Room 10. Also test Quit in Standalone or the packaged build.

Verification here: UHT and separate MSVC compilation of title code and tests
passed. The title test now constructs the actual widget tree, verifies two
non-focusable SButtons, and uses SButton::SimulateClick on Begin to verify its
binding starts the transition. Full linking, automation execution and real mouse
clicks in the game remain pending your rebuild and Play test.

This replaces the custom input approach in TitleResponsive_Steps.md.
