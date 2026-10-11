"""Run in Unreal's Output Log. Creates a red emissive beam material, preserving existing assets."""
import unreal

path = '/Game/GhostOfYourself/Materials'
name = 'M_SecurityBeam'
asset_path = path + '/' + name
if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
    unreal.log('M_SecurityBeam already exists; preserved it.')
else:
    unreal.EditorAssetLibrary.make_directory(path)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, path, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError('Could not create beam material')
    material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    lib = unreal.MaterialEditingLibrary
    color = lib.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -300, 0)
    color.set_editor_property('constant', unreal.LinearColor(5.0, 0.02, 0.01, 1.0))
    if not lib.connect_material_property(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError('Could not connect beam emissive color')
    lib.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError('Could not save beam material')
    unreal.log('Created M_SecurityBeam. Assign it to GhostSecurityBeam > BeamMesh > Materials > Element 0.')
