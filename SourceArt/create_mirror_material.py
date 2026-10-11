"""Run in Unreal Output Log. Creates a simple reflective material; preserves an existing asset."""
import unreal

PATH='/Game/GhostOfYourself/Materials'
NAME='M_RoomMirror'

def create():
    if unreal.EditorAssetLibrary.does_asset_exist(PATH+'/'+NAME):
        unreal.log('M_RoomMirror already exists; preserved it.')
        return
    unreal.EditorAssetLibrary.make_directory(PATH)
    material=unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME,PATH,unreal.Material,unreal.MaterialFactoryNew())
    if not material:raise RuntimeError('Could not create mirror material')
    lib=unreal.MaterialEditingLibrary
    base=lib.create_material_expression(material,unreal.MaterialExpressionConstant3Vector,-300,-100)
    base.set_editor_property('constant',unreal.LinearColor(.92,.92,.92,1))
    metallic=lib.create_material_expression(material,unreal.MaterialExpressionConstant,-300,60)
    metallic.set_editor_property('r',1.0)
    roughness=lib.create_material_expression(material,unreal.MaterialExpressionConstant,-300,180)
    roughness.set_editor_property('r',0.0)
    for expression,property_name in [(base,unreal.MaterialProperty.MP_BASE_COLOR),
        (metallic,unreal.MaterialProperty.MP_METALLIC),(roughness,unreal.MaterialProperty.MP_ROUGHNESS)]:
        if not lib.connect_material_property(expression,'',property_name):raise RuntimeError('Could not connect mirror material property')
    lib.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):raise RuntimeError('Could not save material')
    unreal.log('Created M_RoomMirror. Assign it to a thin wall panel in Room 0.')

create()
