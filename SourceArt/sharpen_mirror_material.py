"""Run inside Unreal: update only the mirror's roughness to zero."""
import unreal

material=unreal.load_asset('/Game/GhostOfYourself/Materials/M_RoomMirror')
if not material:raise RuntimeError('M_RoomMirror was not found.')
lib=unreal.MaterialEditingLibrary
roughness=lib.create_material_expression(material,unreal.MaterialExpressionConstant,-100,220)
roughness.set_editor_property('r',0.0)
if not lib.connect_material_property(roughness,'',unreal.MaterialProperty.MP_ROUGHNESS):
    raise RuntimeError('Could not connect mirror roughness')
lib.recompile_material(material)
if not unreal.EditorAssetLibrary.save_loaded_asset(material):raise RuntimeError('Could not save mirror material')
unreal.log('Mirror roughness set to 0 for the sharpest material reflection. Renderer quality still affects clarity.')
