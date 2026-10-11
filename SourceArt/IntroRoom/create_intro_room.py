"""Run inside Unreal after the full C++ rebuild. Creates a new level; never replaces an existing one."""
import unreal, pathlib, json, textwrap

ROOT=pathlib.Path(__file__).resolve().parent
MAP='/Game/GhostOfYourself/Maps/L_Intro'
VOICE='/Game/GhostOfYourself/Audio/Intro_RobotWelcome'

def load_first(paths):
    for path in paths:
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            return unreal.load_asset(path)
    return None

def create_intro():
    if unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages() or unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages():
        raise RuntimeError('Save All first. No new level was opened and no existing changes were discarded.')
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        unreal.log_warning('L_Intro already exists; preserving it. No actors or assets were changed.')
        return
    native_intro=unreal.load_class(None,'/Script/GhostOfYourself.GhostIntroController')
    native_gate=unreal.load_class(None,'/Script/GhostOfYourself.GhostGate')
    if not native_intro or not native_gate:
        raise RuntimeError('Native intro class is unavailable. Close Unreal and run Rebuild-GhostIntro.ps1 first.')
    sound=load_first([VOICE])
    if not sound:
        unreal.EditorAssetLibrary.make_directory('/Game/GhostOfYourself/Audio')
        task=unreal.AssetImportTask()
        task.filename=str(ROOT/'Audio'/'Intro_RobotWelcome.wav')
        task.destination_path='/Game/GhostOfYourself/Audio'
        task.destination_name='Intro_RobotWelcome'
        task.automated=True;task.replace_existing=False;task.save=False
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        sound=unreal.load_asset(VOICE)
        if not sound: raise RuntimeError('Voice import failed. Existing maps were not changed.')
        narration=json.loads((ROOT/'Narration.json').read_text())
        cues=[]
        for index,line in enumerate(narration['cues']):
            cue=unreal.SubtitleCue()
            text='\n'.join(textwrap.wrap(line['text'],width=64))
            if index==0:text+='\n[Enter to skip introduction]'
            cue.set_editor_property('text',text)
            cue.set_editor_property('time',line['time'])
            cues.append(cue)
        sound.set_editor_property('subtitles',cues)
        sound.set_editor_property('subtitle_priority',10000.0)
        # Preserve measured cue timestamps and show one current cue, rather than the whole script.
        sound.set_editor_property('manual_word_wrap',True)
        sound.set_editor_property('single_line',True)
        sound.set_editor_property('looping',False)
        unreal.EditorAssetLibrary.save_loaded_asset(sound)

    unreal.EditorAssetLibrary.make_directory('/Game/GhostOfYourself/Maps')
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not levels.new_level(MAP): raise RuntimeError('Could not create L_Intro.')
    cube=unreal.load_asset('/Engine/BasicShapes/Cube')
    ivory=load_first(['/Game/GhostOfYourself/Art/SM_Lab_WallPanel_300/Materials/M_Lab_Ivory'])
    steel=load_first(['/Game/GhostOfYourself/Art/SM_Lab_FloorTile_300/Materials/M_Lab_Steel'])
    graphite=load_first(['/Game/GhostOfYourself/Art/SM_Lab_WallPanel_300/Materials/M_Lab_Graphite'])

    def spawn(cls,label,position,rotation=(0,0,0)):
        actor=actors.spawn_actor_from_class(cls,unreal.Vector(*position),unreal.Rotator(*rotation))
        if not actor: raise RuntimeError('Could not spawn '+label)
        actor.set_actor_label(label)
        return actor

    def block(label,position,dimensions,material=None):
        actor=spawn(unreal.StaticMeshActor,label,position)
        component=actor.get_component_by_class(unreal.StaticMeshComponent)
        component.set_static_mesh(cube)
        component.set_collision_profile_name('BlockAll')
        actor.set_actor_scale3d(unreal.Vector(*(d/100 for d in dimensions)))
        if material:component.set_material(0,material)
        return actor

    block('Intro_Floor',(0,170,-10),(940,1380,20),steel)
    block('Intro_Ceiling',(0,170,310),(940,1380,20),ivory)
    block('Intro_WallLeft',(-460,170,150),(20,1380,300),ivory)
    block('Intro_WallRight',(460,170,150),(20,1380,300),ivory)
    block('Intro_WallBack',(0,-510,150),(940,20,300),ivory)
    block('Intro_DoorWallLeft',(-278.75,500,150),(342.5,20,300),ivory)
    block('Intro_DoorWallRight',(278.75,500,150),(342.5,20,300),ivory)
    block('Intro_DoorHeader',(0,500,280),(215,20,40),ivory)
    block('Intro_PassageEnd',(0,860,150),(940,20,300),graphite)

    gate=spawn(native_gate,'Intro_Gate',(0,500,0))
    panel=load_first(['/Game/GhostOfYourself/Art/SM_Lab_GatePanel/StaticMeshes/SM_Lab_GatePanel'])
    if panel:
        component=gate.get_editor_property('gate_mesh');component.set_static_mesh(panel)
        component.set_relative_scale3d(unreal.Vector(1,1,1))
        component.set_relative_location(unreal.Vector(0,0,125))
    frame=load_first(['/Game/GhostOfYourself/Art/SM_Lab_GateFrame/StaticMeshes/SM_Lab_GateFrame'])
    if frame:
        actor=spawn(unreal.StaticMeshActor,'Intro_DoorFrame',(0,500,0))
        component=actor.get_component_by_class(unreal.StaticMeshComponent);component.set_static_mesh(frame)
        component.set_collision_profile_name('NoCollision')

    start=spawn(unreal.PlayerStart,'Intro_PlayerStart',(0,-280,100),(0,90,0))
    intro=spawn(native_intro,'Intro_Controller',(0,690,0))
    intro.set_editor_property('gate_ref',gate)
    intro.set_editor_property('introduction_audio',sound)
    intro.set_editor_property('assessment_level','/Game/GhostOfYourself/Maps/L_TestRoom')
    for index,position in enumerate([(-180,-220,245),(180,300,245)]):
        light=spawn(unreal.PointLight,'Intro_Light'+str(index),position)
        component=light.get_component_by_class(unreal.PointLightComponent)
        component.set_editor_property('intensity',2500.0)
        component.set_editor_property('attenuation_radius',1000.0)
        component.set_editor_property('cast_shadows',False)

    for mesh_name,label,position,rotation in [
        ('SM_Lab_Console','Intro_Console',(-350,-60,0),(0,90,0)),
        ('SM_Lab_StorageCabinet','Intro_Cabinet',(350,-350,0),(0,0,0))]:
        mesh=load_first(['/Game/GhostOfYourself/Art/'+mesh_name+'/StaticMeshes/'+mesh_name])
        if mesh:
            actor=spawn(unreal.StaticMeshActor,label,position,rotation)
            component=actor.get_component_by_class(unreal.StaticMeshComponent);component.set_static_mesh(mesh)
            component.set_collision_profile_name('NoCollision')

    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    game_mode=unreal.load_class(None,'/Game/FirstPerson/Blueprints/BP_FirstPersonGameMode.BP_FirstPersonGameMode_C')
    if game_mode:world.get_world_settings().set_editor_property('default_game_mode',game_mode)
    if not levels.save_current_level():raise RuntimeError('Level was created but could not be saved. Save it manually.')
    unreal.log('L_Intro created. Press Play to test narration, then enter the open door. Set Game Default Map to L_Intro after testing.')

create_intro()
