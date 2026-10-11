"""Run in Unreal after the native rebuild. Preserves existing maps/assets; config changes happen last."""
import unreal, pathlib, re, shutil

ROOT=pathlib.Path(__file__).resolve().parent
PROJECT_ROOT=pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve().parent
ART_ROOT=PROJECT_ROOT/'SourceArt'/'TitleScreen'
TITLE='/Game/GhostOfYourself/Maps/L_Title'
INTRO='/Game/GhostOfYourself/Maps/L_Intro'
TEST='/Game/GhostOfYourself/Maps/L_TestRoom'
TEXTURE_PATH='/Game/GhostOfYourself/UI/Title'

def update_section(path,section,settings=None,additions=None,removals=None):
    settings=settings or {};additions=additions or [];removals=removals or []
    text=path.read_text(encoding='utf-8-sig') if path.exists() else ''
    match=re.search(r'(?m)^\['+re.escape(section)+r'\][ \t]*\r?$',text)
    if match:
        next_section=re.search(r'(?m)^\[',text[match.end():])
        end=match.end()+next_section.start() if next_section else len(text)
        body=text[match.end():end]
        body=''.join(line for line in body.splitlines(keepends=True) if line.strip() not in removals)
        for key,value in settings.items():
            pattern=r'(?m)^'+re.escape(key)+r'=.*$'
            replacement=key+'='+value
            body=re.sub(pattern,lambda m:replacement,body) if re.search(pattern,body) else body.rstrip()+'\n'+replacement+'\n'
        for line in additions:
            if line not in body:body=body.rstrip()+'\n'+line+'\n'
        text=text[:match.end()]+'\n'+body.strip()+'\n\n'+text[end:]
    else:
        text=text.rstrip()+'\n\n['+section+']\n'+'\n'.join([k+'='+v for k,v in settings.items()]+additions)+'\n\n'
    backup=path.with_name(path.name+'.before-title-menu.bak')
    if path.exists() and not backup.exists():shutil.copy2(path,backup)
    path.write_text(text,encoding='utf-8')

def import_texture(filename,name):
    asset_path=TEXTURE_PATH+'/'+name
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):return unreal.load_asset(asset_path)
    task=unreal.AssetImportTask()
    source=(ART_ROOT/filename) if (ART_ROOT/filename).exists() else (ROOT/filename)
    task.filename=str(source);task.destination_path=TEXTURE_PATH;task.destination_name=name
    task.automated=True;task.replace_existing=False;task.save=False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture=unreal.load_asset(asset_path)
    if not texture:raise RuntimeError('Could not import '+filename)
    texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('srgb',True)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture):raise RuntimeError('Could not save '+asset_path)
    return texture

def setup():
    if unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages() or unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages():
        raise RuntimeError('Save All before running this script. Existing unsaved work was preserved.')
    mode=unreal.load_class(None,'/Script/GhostOfYourself.GhostTitleGameMode')
    if not mode:raise RuntimeError('Run Rebuild-GhostTitle.ps1 with Unreal closed, then reopen before setup.')
    if not unreal.EditorAssetLibrary.does_asset_exist(TEST):raise RuntimeError('L_TestRoom is missing; startup settings were not changed.')
    unreal.EditorAssetLibrary.make_directory(TEXTURE_PATH)
    logo=import_texture('GhostOfYourself_Logo.png','T_GhostTitleLogo')
    background=import_texture('Title_Background.png','T_GhostTitleBackground')
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not unreal.EditorAssetLibrary.does_asset_exist(TITLE):
        if not levels.new_level(TITLE):raise RuntimeError('Could not create L_Title.')
        world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        world.get_world_settings().set_editor_property('default_game_mode',mode)
        if not levels.save_current_level():raise RuntimeError('Could not save L_Title; startup settings were not changed.')
    else:
        # Preserve an existing level; refuse to silently replace another title implementation.
        if not levels.load_level(TITLE):raise RuntimeError('Could not load existing L_Title.')
        world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        if world.get_world_settings().get_editor_property('default_game_mode') != mode:
            raise RuntimeError('Existing L_Title uses another GameMode. Preserved it; assign GhostTitleGameMode in World Settings, save, then rerun.')
    config=pathlib.Path(unreal.Paths.project_config_dir())
    update_section(config/'DefaultEngine.ini','/Script/EngineSettings.GameMapsSettings',
        {'GameDefaultMap':TITLE+'.L_Title'})
    update_section(config/'DefaultGame.ini','/Script/UnrealEd.ProjectPackagingSettings',additions=[
        '+MapsToCook=(FilePath="'+TITLE+'")',
        '+MapsToCook=(FilePath="'+TEST+'")',
        '+DirectoriesToAlwaysCook=(Path="'+TEXTURE_PATH+'")'],removals=([] if unreal.EditorAssetLibrary.does_asset_exist(INTRO) else [
            '+MapsToCook=(FilePath="'+INTRO+'")']))
    # Update the current editor's settings cache as well as next-launch configuration.
    unreal.GameMapsSettings.set_game_default_map(TITLE+'.L_Title')
    packaging_class=getattr(unreal,'ProjectPackagingSettings',None)
    if packaging_class:
        packaging=unreal.get_default_object(packaging_class)
        maps=list(packaging.get_editor_property('maps_to_cook'))
        if not unreal.EditorAssetLibrary.does_asset_exist(INTRO):
            maps=[entry for entry in maps if entry.get_editor_property('file_path') != INTRO]
        for path in (TITLE,TEST):
            if not any(entry.get_editor_property('file_path')==path for entry in maps):
                entry=unreal.FilePath();entry.set_editor_property('file_path',path);maps.append(entry)
        packaging.set_editor_property('maps_to_cook',maps)
        directories=list(packaging.get_editor_property('directories_to_always_cook'))
        if not any(entry.get_editor_property('path')==TEXTURE_PATH for entry in directories):
            entry=unreal.DirectoryPath();entry.set_editor_property('path',TEXTURE_PATH);directories.append(entry)
        packaging.set_editor_property('directories_to_always_cook',directories)
    unreal.log('Title setup complete: Game Default Map = L_Title. BEGIN goes straight to L_TestRoom. Existing Editor Startup Map was preserved.')

setup()
