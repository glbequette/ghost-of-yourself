"""Executed by the rebuild script in Unreal; imports only the new music asset."""
from pathlib import Path
import unreal

directory = '/Game/GhostOfYourself/Audio'
asset_path = directory + '/GOY_LabMusic'
source = Path(__file__).with_name('GOY_LabMusic.wav')
if not source.is_file():
    raise RuntimeError('Music WAV is missing: ' + str(source))
if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
    music = unreal.EditorAssetLibrary.load_asset(asset_path)
    unreal.log('Preserving existing GOY_LabMusic audio content.')
else:
    unreal.EditorAssetLibrary.make_directory(directory)
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(source))
    task.set_editor_property('destination_path', directory)
    task.set_editor_property('destination_name', 'GOY_LabMusic')
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', False)
    task.set_editor_property('save', False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    music = unreal.EditorAssetLibrary.load_asset(asset_path)
if not isinstance(music, unreal.SoundWave):
    raise RuntimeError('GOY_LabMusic must be a SoundWave asset')
music.set_editor_property('looping', True)
if not unreal.EditorAssetLibrary.save_loaded_asset(music):
    raise RuntimeError('Could not save looping music asset')
unreal.log('GOY_LabMusic imported and configured to loop.')
