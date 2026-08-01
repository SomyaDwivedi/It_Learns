import os

import unreal


PROJECT_SAVED_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())
IMPORT_ROOT = os.path.join(PROJECT_SAVED_DIR, "Codex", "HospitalImport_20260731")
MESH_SOURCE_DIR = os.path.join(IMPORT_ROOT, "Meshes")
TEXTURE_SOURCE_DIR = os.path.join(IMPORT_ROOT, "Textures", "UE5_textures")

MESH_DESTINATION = "/Game/Hospital/Imported/Meshes"
TEXTURE_DESTINATION = "/Game/Hospital/Imported/Textures"
MATERIAL_DESTINATION = "/Game/Hospital/Materials"


def require_directory(path):
    if not os.path.isdir(path):
        raise RuntimeError("Required hospital import directory is missing: {}".format(path))


def import_meshes(asset_tools):
    tasks = []
    for filename in sorted(os.listdir(MESH_SOURCE_DIR)):
        if not filename.lower().endswith(".fbx"):
            continue

        options = unreal.FbxImportUI()
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_as_skeletal", False)
        options.set_editor_property("import_materials", False)
        options.set_editor_property("import_textures", False)

        static_mesh_options = options.get_editor_property("static_mesh_import_data")
        static_mesh_options.set_editor_property("combine_meshes", True)
        static_mesh_options.set_editor_property("auto_generate_collision", True)
        static_mesh_options.set_editor_property("generate_lightmap_u_vs", True)
        static_mesh_options.set_editor_property("remove_degenerates", True)

        task = unreal.AssetImportTask()
        task.set_editor_property("filename", os.path.join(MESH_SOURCE_DIR, filename))
        task.set_editor_property("destination_path", MESH_DESTINATION)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        task.set_editor_property("options", options)
        tasks.append(task)

    asset_tools.import_asset_tasks(tasks)
    return tasks


def import_textures(asset_tools):
    tasks = []
    for root, _, filenames in os.walk(TEXTURE_SOURCE_DIR):
        group_name = os.path.basename(root)
        for filename in sorted(filenames):
            if not filename.lower().endswith((".png", ".tga")):
                continue

            task = unreal.AssetImportTask()
            task.set_editor_property("filename", os.path.join(root, filename))
            task.set_editor_property("destination_path", "{}/{}".format(TEXTURE_DESTINATION, group_name))
            task.set_editor_property("automated", True)
            task.set_editor_property("replace_existing", True)
            task.set_editor_property("save", True)
            tasks.append(task)

    asset_tools.import_asset_tasks(tasks)
    return tasks


def load_asset(asset_path):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        raise RuntimeError("Unable to load imported asset: {}".format(asset_path))
    return asset


def find_texture(group_name, name_fragment):
    directory = "{}/{}".format(TEXTURE_DESTINATION, group_name)
    for asset_path in unreal.EditorAssetLibrary.list_assets(directory, recursive=False, include_folder=False):
        if name_fragment.lower() in os.path.basename(asset_path).lower():
            return load_asset(asset_path)
    return None


def configure_texture(texture, texture_kind):
    if not texture:
        return

    if texture_kind == "normal":
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        texture.set_editor_property("srgb", False)
    elif texture_kind in ("orm", "roughness", "metallic"):
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        texture.set_editor_property("srgb", False)

    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)


def create_textured_material(asset_tools, material_name, texture_group):
    material_path = "{}/{}".format(MATERIAL_DESTINATION, material_name)
    existing = unreal.EditorAssetLibrary.load_asset(material_path)
    if existing:
        return existing

    material = asset_tools.create_asset(
        material_name,
        MATERIAL_DESTINATION,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if not material:
        raise RuntimeError("Unable to create material: {}".format(material_path))

    base_color = find_texture(texture_group, "BaseColor")
    normal = find_texture(texture_group, "Normal")
    orm = find_texture(texture_group, "OcclusionRoughnessMetallic")
    roughness = find_texture(texture_group, "Roughness")
    metallic = find_texture(texture_group, "Metallic")

    configure_texture(normal, "normal")
    configure_texture(orm, "orm")
    configure_texture(roughness, "roughness")
    configure_texture(metallic, "metallic")

    if base_color:
        base_node = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionTextureSample, -650, -150
        )
        base_node.set_editor_property("texture", base_color)
        unreal.MaterialEditingLibrary.connect_material_property(
            base_node, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
        )

    if normal:
        normal_node = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionTextureSample, -650, 100
        )
        normal_node.set_editor_property("texture", normal)
        normal_node.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        unreal.MaterialEditingLibrary.connect_material_property(
            normal_node, "RGB", unreal.MaterialProperty.MP_NORMAL
        )

    if orm:
        orm_node = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionTextureSample, -650, 350
        )
        orm_node.set_editor_property("texture", orm)
        orm_node.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        unreal.MaterialEditingLibrary.connect_material_property(
            orm_node, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            orm_node, "G", unreal.MaterialProperty.MP_ROUGHNESS
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            orm_node, "B", unreal.MaterialProperty.MP_METALLIC
        )
    else:
        if roughness:
            roughness_node = unreal.MaterialEditingLibrary.create_material_expression(
                material, unreal.MaterialExpressionTextureSample, -650, 350
            )
            roughness_node.set_editor_property("texture", roughness)
            roughness_node.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
            unreal.MaterialEditingLibrary.connect_material_property(
                roughness_node, "R", unreal.MaterialProperty.MP_ROUGHNESS
            )
        if metallic:
            metallic_node = unreal.MaterialEditingLibrary.create_material_expression(
                material, unreal.MaterialExpressionTextureSample, -650, 550
            )
            metallic_node.set_editor_property("texture", metallic)
            metallic_node.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
            unreal.MaterialEditingLibrary.connect_material_property(
                metallic_node, "R", unreal.MaterialProperty.MP_METALLIC
            )

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def assign_material(mesh_name, material):
    mesh_path = "{}/{}".format(MESH_DESTINATION, mesh_name)
    mesh = load_asset(mesh_path)
    static_materials = mesh.get_editor_property("static_materials")
    for material_index in range(len(static_materials)):
        mesh.set_material(material_index, material)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)


def create_and_assign_materials(asset_tools):
    material_groups = {
        "M_Hospital_Walls": ("Walls_1", ["pillar", "tile_corner", "tile_wall", "tile_wall_half", "tile_window"]),
        "M_Hospital_Doorway": ("Doorway_1", ["tile_doorway_1", "tile_doorway_2"]),
        "M_Hospital_Floor": ("Floors_1", ["floor_tile_1", "floor_tile_2"]),
        "M_Hospital_Ceiling": ("ceiling_1", ["ceiling_tile"]),
        "M_Hospital_CeilingLight": ("Ceiling_light", ["ceiling_light"]),
        "M_Hospital_Door": ("Door_1", ["door_1", "door_2"]),
        "M_Hospital_Furniture": (
            "Chairs_table_1",
            ["bench", "cabinet_1", "cabinet_2", "cabinet_3", "chair", "table"],
        ),
        "M_Hospital_Bed": ("bed", ["bed"]),
        "M_Hospital_Wheelchair": ("wheel_chair", ["wheel_chair"]),
        "M_Hospital_IVBag": ("iv_bag", ["IV_Bag"]),
        "M_Hospital_IVPole": ("iv_bag_holder", ["IV_Bag_holder"]),
        "M_Hospital_Magazine": ("magazine", ["Magazine1"]),
        "M_Hospital_ExitSign": ("exit_sign", ["Exit_sign"]),
    }

    for material_name, (texture_group, mesh_names) in material_groups.items():
        material = create_textured_material(asset_tools, material_name, texture_group)
        for mesh_name in mesh_names:
            assign_material(mesh_name, material)


def report_mesh_bounds():
    for asset_path in unreal.EditorAssetLibrary.list_assets(MESH_DESTINATION, recursive=False, include_folder=False):
        mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not isinstance(mesh, unreal.StaticMesh):
            continue
        bounds = mesh.get_bounds()
        unreal.log(
            "HOSPITAL_BOUNDS|{}|ORIGIN={}|EXTENT={}|RADIUS={}".format(
                mesh.get_name(), bounds.origin, bounds.box_extent, bounds.sphere_radius
            )
        )


def main():
    require_directory(MESH_SOURCE_DIR)
    require_directory(TEXTURE_SOURCE_DIR)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    existing_meshes = unreal.EditorAssetLibrary.list_assets(
        MESH_DESTINATION, recursive=False, include_folder=False
    )
    existing_textures = unreal.EditorAssetLibrary.list_assets(
        TEXTURE_DESTINATION, recursive=True, include_folder=False
    )

    mesh_tasks = import_meshes(asset_tools) if len(existing_meshes) < 25 else []
    texture_tasks = import_textures(asset_tools) if len(existing_textures) < 42 else []
    create_and_assign_materials(asset_tools)
    report_mesh_bounds()

    unreal.EditorAssetLibrary.save_directory("/Game/Hospital", only_if_is_dirty=False, recursive=True)
    unreal.log(
        "HOSPITAL_IMPORT_COMPLETE|MESH_TASKS={}|TEXTURE_TASKS={}".format(
            len(mesh_tasks), len(texture_tasks)
        )
    )


main()
