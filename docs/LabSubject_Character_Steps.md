# Lab subject character

An original stylized subject in a simple teal jumpsuit, dark gloves and boots, short hair and an ID patch. About 178 cm tall. Includes a full body, matching first-person arms, an 18-bone rig and basic looping idle/walk animations. No external models or textures used.

This is a character art kit for your existing BP_FirstPersonCharacter. Keep that Blueprint, movement, camera, input, capsule and recording code. The character uses its own skeleton, so the mannequin's animation Blueprints cannot drive it directly. No project map or binary asset was changed automatically.

## 1. Import the full body first

Unreal can remain open for this asset setup. Stop Play and save your current working character Blueprint first.

1. Create `Content/GhostOfYourself/Characters/LabSubject` in Content Browser.
2. Import `Meshes/SKM_LabSubject.fbx` as a **Skeletal Mesh**. Leave the Skeleton selection empty to create a new subject skeleton. Import materials; skip animations for this file. Do not select SK_Mannequin.
3. Open the new skeletal mesh and check that the character is upright and approximately 178 cm tall. Use the default FBX units initially. Do not scale the player capsule to compensate for an incorrect import scale.
4. Import `Animations/A_LabSubject_Idle.fbx` and `Animations/A_LabSubject_Walk.fbx` as **animations only**: disable importing mesh and select the newly created subject skeleton. These files contain the body for interchange compatibility, but it does not need to be imported again.
5. Open both animation sequences. Verify that Idle breathes gently and Walk moves the legs/arms. These are basic in-place clips; there is no root-motion locomotion, crouch, jump, interaction gesture or facial animation.

## 2. Make a small movement Animation Blueprint

1. Create a **Blend Space 1D** using the subject skeleton. Name it `BS_LabSubject_Move`.
2. Set its horizontal axis name to `Speed`, minimum 0 and maximum to your character's current **Max Walk Speed** (commonly 600 cm/s; inspect your character rather than changing its speed).
3. Put the Idle sequence at Speed 0 and Walk at the maximum. Walking pace may need tuning after import. This first pass blends walking with idle rather than supporting directional strafing animations.
4. Create an **Animation Blueprint** using the same subject skeleton. Name it `ABP_LabSubject`.
5. Add a float variable named `Speed`, default 0.
6. In its Event Graph, connect **Event Blueprint Update Animation → Is Valid → Set Speed**. Use **Try Get Pawn Owner** as the Is Valid input. For the Speed value, take that pawn's **Get Velocity → Vector Length XY**. On the invalid branch, set Speed to 0 so the editor preview remains idle.
7. In AnimGraph, add `BS_LabSubject_Move`, connect the Speed variable to its Speed input, enable looping, and connect its pose to **Output Pose**.
8. Compile and save.

## 3. Replace the inherited body mesh

1. Open BP_FirstPersonCharacter and select its inherited **Mesh (CharacterMesh0)** component. This is the body returned by GetMesh and copied by your C++ ghost.
2. Write down its existing mesh, animation class, relative transform and visibility settings so you can restore them if needed.
3. Set Skeletal Mesh Asset to `SKM_LabSubject`, Animation Mode to **Use Animation Blueprint**, and Anim Class to `ABP_LabSubject`.
4. Clear old per-component material overrides so the subject's own imported materials appear.
5. Keep the existing capsule and camera settings. Adjust only the body mesh's relative position/rotation if needed: feet should meet the capsule bottom, and the character should face the same direction as movement. The FBX faces authored -Y; importer axis conversion can affect the required yaw. Verify in the Blueprint viewport instead of assuming the mannequin's old yaw is correct.
6. For a first-person body hidden from its owner, enable **Owner No See** on this full-body component. It can still animate and the spawned ghost remains visible. Set its **Visibility Based Anim Tick Option** to **Always Tick Pose and Refresh Bones** so hidden-body animation is available for recording.
7. Compile/save, Play, then record a short walk. Your blue translucent ghost should now use the subject body and its recorded poses.

Do not hide the head bone on this source body to solve first-person clipping: the ghost records its bone transforms too. Use body visibility and the separate arms mesh instead.

## 4. Replace the first-person arms

1. Import `Meshes/SKM_LabSubject_Arms.fbx` as a skeletal mesh, selecting the same subject skeleton. Skip animations; use its imported materials.
2. In BP_FirstPersonCharacter, select the skeletal mesh component that currently supplies the owner's first-person arms. Keep its attachment, camera and first-person rendering settings.
3. Record its current settings, then assign `SKM_LabSubject_Arms` and `ABP_LabSubject` (Use Animation Blueprint). Clear old material overrides. The old mannequin Copy Pose animation Blueprint uses a different skeleton and should not remain assigned to the new arms.
4. Preserve the existing component transform initially, then adjust its relative position while checking the first-person view. The arms mesh uses full-body coordinates with a floor-level root, so camera-relative components may need a downward offset. Avoid camera clipping and keep the hands visible near the bottom of the view.
5. This version has hanging arms and basic movement sway; it does not yet include a reaching/pushing animation for E. Button interaction still works through the existing code.

## 5. Verify gameplay before saving the final character setup

- Walk and stop: the subject should switch smoothly between walking and idle.
- Look around in first person: no face or torso should obscure the camera; visible hands should match the jumpsuit.
- Record, save and restart: the blue ghost should have its head, boots, gloves and correct pose timing.
- Walk through the ghost: it must remain collision-disabled.
- Complete both the plate and simultaneous-button puzzles and verify room resets still work.

## Files and verification

`LabSubject.blend` includes editable geometry, rig and both actions. `Source/build_lab_subject.py` regenerates everything with Blender 5. `Source/validate_exports.py` reimports the four FBX files and checks rig/weights/animation presence, then renders a walk pose.

The generated geometry has finite, weighted vertices; all four FBX exports were reimported successfully in Blender with the expected rig and animation tracks. Preview images are Blender renders of the actual mesh. Unreal import, camera alignment and gameplay animation are not verified. An attempted read-only Unreal commandlet export produced no reference file, so this kit uses its own rig rather than claiming mannequin skeleton compatibility. No C++ changes or rebuild are required for these asset/Blueprint hookup steps.
