import math

import unreal


MAP_PATH = "/Game/Hospital/Maps/Lvl_Hospital"
MESH_ROOT = "/Game/Hospital/Imported/Meshes"
GENERATED_TAG = unreal.Name("HospitalGenerated")

LEVEL_EDITOR_SUBSYSTEM = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
UNREAL_EDITOR_SUBSYSTEM = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
ACTOR_SUBSYSTEM = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

FUSE_BOX_CLASS = unreal.load_class(None, "/Script/IT_Learns.HorrorFuseBox")
POWER_CONTROLLER_CLASS = unreal.load_class(None, "/Script/IT_Learns.HorrorPowerController")
if not FUSE_BOX_CLASS or not POWER_CONTROLLER_CLASS:
    raise RuntimeError("Hospital power-puzzle classes are unavailable; compile the editor target first")

FLOOR_Z = 10.0
WALL_TOP_Z = 250.0
GRID_SIZE = 200.0


def load_mesh(mesh_name):
    mesh = unreal.EditorAssetLibrary.load_asset("{}/{}".format(MESH_ROOT, mesh_name))
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("Missing hospital static mesh: {}".format(mesh_name))
    return mesh


MESHES = {
    name: load_mesh(name)
    for name in (
        "Exit_sign",
        "IV_Bag",
        "IV_Bag_holder",
        "Magazine1",
        "bed",
        "bench",
        "cabinet_1",
        "cabinet_2",
        "cabinet_3",
        "ceiling_light",
        "ceiling_tile",
        "chair",
        "door_1",
        "door_2",
        "floor_tile_1",
        "floor_tile_2",
        "pillar",
        "table",
        "tile_corner",
        "tile_doorway_1",
        "tile_doorway_2",
        "tile_wall",
        "tile_wall_half",
        "tile_window",
        "wheel_chair",
    )
}
MESHES["navigation_floor"] = unreal.EditorAssetLibrary.load_asset(
    "/Engine/BasicShapes/Cube.Cube"
)
if not isinstance(MESHES["navigation_floor"], unreal.StaticMesh):
    raise RuntimeError("Missing Engine navigation floor mesh")

SPAWN_DEBUG_COUNT = 0


def prepare_level():
    unreal.log("HOSPITAL_STAGE|PREPARE_LEVEL_BEGIN")
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        LEVEL_EDITOR_SUBSYSTEM.load_level(MAP_PATH)
        unreal.log("HOSPITAL_STAGE|LEVEL_LOADED")
        for actor in ACTOR_SUBSYSTEM.get_all_level_actors():
            if GENERATED_TAG in actor.get_editor_property("tags"):
                ACTOR_SUBSYSTEM.destroy_actor(actor)
    else:
        if not LEVEL_EDITOR_SUBSYSTEM.new_level(MAP_PATH, False):
            raise RuntimeError("Unable to create hospital map: {}".format(MAP_PATH))

    world = UNREAL_EDITOR_SUBSYSTEM.get_editor_world()
    unreal.log("HOSPITAL_STAGE|WORLD_ACQUIRED")
    world_settings = world.get_world_settings()
    unreal.log("HOSPITAL_STAGE|WORLD_SETTINGS_ACQUIRED")
    horror_game_mode = unreal.load_class(
        None,
        "/Game/Variant_Horror/Blueprints/BP_HorrorGameMode.BP_HorrorGameMode_C",
    )
    unreal.log("HOSPITAL_STAGE|GAME_MODE_LOADED")
    if horror_game_mode:
        world_settings.set_editor_property("default_game_mode", horror_game_mode)
    unreal.log("HOSPITAL_STAGE|GAME_MODE_ASSIGNED")
    return world


def mark_generated(actor, label, folder, extra_tags=None):
    tags = [GENERATED_TAG]
    for gameplay_tag in extra_tags or []:
        tag_name = unreal.Name(str(gameplay_tag))
        if tag_name not in tags:
            tags.append(tag_name)
    actor.set_editor_property("tags", tags)
    actor.set_actor_label(label)
    try:
        actor.set_folder_path(folder)
    except Exception:
        pass
    return actor


def spawn_mesh(
    mesh_name,
    location,
    yaw=0.0,
    label=None,
    folder="Hospital/Architecture",
    collision_profile="BlockAll",
):
    global SPAWN_DEBUG_COUNT
    SPAWN_DEBUG_COUNT += 1
    debug_first_spawn = SPAWN_DEBUG_COUNT == 1
    mesh = MESHES[mesh_name]
    if debug_first_spawn:
        unreal.log("HOSPITAL_STAGE|FIRST_SPAWN_BEFORE_ACTOR")
    actor = ACTOR_SUBSYSTEM.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(*location),
        unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0),
    )
    if not actor:
        raise RuntimeError("Unable to spawn mesh actor: {}".format(mesh_name))
    if debug_first_spawn:
        unreal.log("HOSPITAL_STAGE|FIRST_SPAWN_ACTOR_CREATED")

    component = actor.get_editor_property("static_mesh_component")
    if debug_first_spawn:
        unreal.log("HOSPITAL_STAGE|FIRST_SPAWN_COMPONENT_ACQUIRED")
    component.set_static_mesh(mesh)
    if debug_first_spawn:
        unreal.log("HOSPITAL_STAGE|FIRST_SPAWN_MESH_ASSIGNED")
    component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    component.set_editor_property("cast_shadow", True)
    try:
        component.set_collision_profile_name(collision_profile)
        if collision_profile == "NoCollision":
            component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    except Exception:
        pass

    if debug_first_spawn:
        unreal.log("HOSPITAL_STAGE|FIRST_SPAWN_COMPONENT_CONFIGURED")

    actor = mark_generated(
        actor,
        label or "SM_{}".format(mesh_name),
        folder,
    )
    if debug_first_spawn:
        unreal.log("HOSPITAL_STAGE|FIRST_SPAWN_COMPLETE")
    return actor


def mesh_floor_location(mesh_name, x, y, yaw=0.0):
    bounds = MESHES[mesh_name].get_bounds()
    local_bottom = bounds.origin.z - bounds.box_extent.z
    return spawn_mesh(
        mesh_name,
        (x, y, FLOOR_Z - local_bottom),
        yaw=yaw,
        folder="Hospital/Props",
    )


def build_floor_and_ceiling():
    actor_count = 0
    for x_index, x in enumerate(range(-1600, 1601, 200)):
        for y_index, y in enumerate(range(-1200, 1201, 200)):
            floor_mesh = "floor_tile_1" if (x_index + y_index) % 2 == 0 else "floor_tile_2"
            spawn_mesh(
                floor_mesh,
                (float(x), float(y), 0.0),
                label="Floor_{}_{}".format(x_index, y_index),
                folder="Hospital/Architecture/Floor",
            )
            spawn_mesh(
                "ceiling_tile",
                (float(x), float(y), 260.0),
                label="Ceiling_{}_{}".format(x_index, y_index),
                folder="Hospital/Architecture/Ceiling",
            )
            actor_count += 2
    return actor_count


def build_wall_line_x(y, door_positions=None, window_positions=None, label_prefix="WallX"):
    door_positions = set(door_positions or [])
    window_positions = set(window_positions or [])
    actor_count = 0
    for x in range(-1600, 1601, 200):
        if x in door_positions:
            mesh_name = "tile_doorway_1"
        elif x in window_positions:
            mesh_name = "tile_window"
        else:
            mesh_name = "tile_wall"
        spawn_mesh(
            mesh_name,
            (float(x), float(y), 0.0),
            label="{}_{}".format(label_prefix, x),
            folder="Hospital/Architecture/Walls",
            # The imported doorway meshes use broad generated collision that
            # covers part of their opening. Keep the visible frame, but let the
            # neighboring wall tiles define the entrance clearance.
            collision_profile="NoCollision" if x in door_positions else "BlockAll",
        )
        actor_count += 1
    return actor_count


def build_wall_line_y(x, label_prefix="WallY"):
    actor_count = 0
    for y in range(-1200, 1201, 200):
        spawn_mesh(
            "tile_wall",
            (float(x), float(y), 0.0),
            yaw=90.0,
            label="{}_{}".format(label_prefix, y),
            folder="Hospital/Architecture/Walls",
        )
        actor_count += 1
    return actor_count


def build_room_partition(x, north=True):
    actor_count = 0
    direction = 1 if north else -1
    for distance in (300, 500, 700, 900, 1100):
        y = direction * distance
        spawn_mesh(
            "tile_wall",
            (float(x), float(y), 0.0),
            yaw=90.0,
            label="Partition_{}_{}".format(x, y),
            folder="Hospital/Architecture/Partitions",
        )
        actor_count += 1

    y_half = direction * 1250
    spawn_mesh(
        "tile_wall_half",
        (float(x), float(y_half), 0.0),
        yaw=90.0,
        label="PartitionHalf_{}_{}".format(x, y_half),
        folder="Hospital/Architecture/Partitions",
    )
    return actor_count + 1


def build_walls():
    actor_count = 0
    actor_count += build_wall_line_x(
        -1300.0,
        door_positions=[0],
        window_positions=[-1200, -800, 800, 1200],
        label_prefix="ExteriorSouth",
    )
    actor_count += build_wall_line_x(
        1300.0,
        door_positions=[0],
        window_positions=[-1200, -800, 800, 1200],
        label_prefix="ExteriorNorth",
    )
    actor_count += build_wall_line_y(-1700.0, label_prefix="ExteriorWest")
    actor_count += build_wall_line_y(1700.0, label_prefix="ExteriorEast")

    room_doors = [-1200, -400, 400, 1200]
    actor_count += build_wall_line_x(-200.0, door_positions=room_doors, label_prefix="CorridorSouth")
    actor_count += build_wall_line_x(200.0, door_positions=room_doors, label_prefix="CorridorNorth")

    for partition_x in (-800, 0, 800):
        actor_count += build_room_partition(partition_x, north=True)
        actor_count += build_room_partition(partition_x, north=False)

    for x, y, yaw in (
        (-1700, -1300, 0),
        (1700, -1300, 90),
        (-1700, 1300, -90),
        (1700, 1300, 180),
    ):
        spawn_mesh(
            "pillar",
            (float(x), float(y), 0.0),
            yaw=float(yaw),
            label="CornerPillar_{}_{}".format(x, y),
            folder="Hospital/Architecture/Walls",
        )
        actor_count += 1
    return actor_count


def build_room_props():
    actor_count = 0
    room_centers = [-1200, -400, 400, 1200]

    for north in (False, True):
        side = 1 if north else -1
        room_y = side * 760
        exterior_y = side * 1120
        for room_index, room_x in enumerate(room_centers):
            for bed_offset in (-170, 170):
                bed = mesh_floor_location("bed", room_x + bed_offset, room_y, yaw=0.0)
                bed.set_actor_label("PatientBed_{}_{}_{}".format("N" if north else "S", room_index, bed_offset))
                actor_count += 1

            pole = mesh_floor_location("IV_Bag_holder", room_x + 285, room_y + side * 80, yaw=0.0)
            pole.set_actor_label("IVPole_{}_{}".format("N" if north else "S", room_index))
            actor_count += 1

            bag = mesh_floor_location("IV_Bag", room_x + 285, room_y + side * 80, yaw=0.0)
            bag.set_actor_label("IVBag_{}_{}".format("N" if north else "S", room_index))
            actor_count += 1

            cabinet = mesh_floor_location("cabinet_2", room_x - 260, exterior_y, yaw=180.0 if north else 0.0)
            cabinet.set_actor_label("Cabinet_{}_{}".format("N" if north else "S", room_index))
            actor_count += 1

            chair = mesh_floor_location("chair", room_x + 260, exterior_y, yaw=180.0 if north else 0.0)
            chair.set_actor_label("VisitorChair_{}_{}".format("N" if north else "S", room_index))
            actor_count += 1

    reception_table = mesh_floor_location("table", -1050, -1050, yaw=0.0)
    reception_table.set_actor_label("ReceptionDesk")
    reception_chair = mesh_floor_location("chair", -1050, -900, yaw=180.0)
    reception_chair.set_actor_label("ReceptionChair")
    waiting_bench_1 = mesh_floor_location("bench", 850, -1050, yaw=0.0)
    waiting_bench_1.set_actor_label("WaitingBench_A")
    waiting_bench_2 = mesh_floor_location("bench", 1150, -1050, yaw=0.0)
    waiting_bench_2.set_actor_label("WaitingBench_B")
    wheelchair = mesh_floor_location("wheel_chair", 1450, -850, yaw=-35.0)
    wheelchair.set_actor_label("AbandonedWheelchair")
    actor_count += 5

    for x in (-1200, -400, 400, 1200):
        sign = spawn_mesh(
            "Exit_sign",
            (float(x), -185.0, 220.0),
            yaw=0.0,
            label="RoomSign_{}".format(x),
            folder="Hospital/Props/Signs",
        )
        actor_count += 1

    return actor_count


def spawn_point_light(
    location,
    intensity,
    color,
    attenuation=520.0,
    label="HospitalLight",
    gameplay_tags=None,
):
    actor = ACTOR_SUBSYSTEM.spawn_actor_from_class(
        unreal.PointLight,
        unreal.Vector(*location),
        unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0),
    )
    component = actor.get_component_by_class(unreal.PointLightComponent)
    component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    component.set_editor_property("intensity", float(intensity))
    component.set_editor_property("attenuation_radius", float(attenuation))
    component.set_editor_property("light_color", color)
    component.set_editor_property("cast_shadows", True)
    return mark_generated(actor, label, "Hospital/Lighting", gameplay_tags)


def build_lighting():
    actor_count = 0
    cool_white = unreal.Color(r=155, g=175, b=190, a=255)
    sickly_white = unreal.Color(r=155, g=180, b=125, a=255)
    emergency_red = unreal.Color(r=255, g=10, b=6, a=255)

    for x in range(-1400, 1401, 400):
        spawn_mesh(
            "ceiling_light",
            (float(x), 0.0, WALL_TOP_Z),
            label="CorridorFixture_{}".format(x),
            folder="Hospital/Lighting/Fixtures",
        )
        is_emergency = x in (-600, 1000)
        color = emergency_red if is_emergency else cool_white
        intensity = 90.0 if is_emergency else 240.0
        spawn_point_light(
            (float(x), 0.0, 205.0),
            intensity,
            color,
            attenuation=430.0,
            label="CorridorLight_{}".format(x),
            gameplay_tags=[
                "HospitalEmergencyLight" if is_emergency else "HospitalMainsLight"
            ],
        )
        actor_count += 2

    for x in (-1200, -400, 400, 1200):
        for y in (-760, 760):
            spawn_mesh(
                "ceiling_light",
                (float(x), float(y), WALL_TOP_Z),
                label="RoomFixture_{}_{}".format(x, y),
                folder="Hospital/Lighting/Fixtures",
            )
            spawn_point_light(
                (float(x), float(y), 205.0),
                200.0,
                sickly_white,
                attenuation=560.0,
                label="RoomLight_{}_{}".format(x, y),
                gameplay_tags=["HospitalMainsLight"],
            )
            actor_count += 2
    return actor_count


def build_post_process():
    actor = ACTOR_SUBSYSTEM.spawn_actor_from_class(
        unreal.PostProcessVolume,
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0),
    )
    mark_generated(
        actor,
        "HospitalPostProcess",
        "Hospital/Lighting",
        ["HospitalPowerPostProcess"],
    )
    actor.set_editor_property("unbound", True)

    settings = actor.get_editor_property("settings")
    settings.set_editor_property("override_auto_exposure_method", True)
    settings.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
    settings.set_editor_property("override_auto_exposure_bias", True)
    settings.set_editor_property("auto_exposure_bias", -3.0)
    settings.set_editor_property("override_auto_exposure_apply_physical_camera_exposure", True)
    settings.set_editor_property("auto_exposure_apply_physical_camera_exposure", False)
    actor.set_editor_property("settings", settings)
    return 1


def build_player_starts():
    actor_count = 0
    for index, x in enumerate((-1400.0, -1120.0, -840.0, -560.0)):
        actor = ACTOR_SUBSYSTEM.spawn_actor_from_class(
            unreal.PlayerStart,
            unreal.Vector(x, 0.0, 105.0),
            unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0),
        )
        mark_generated(actor, "PlayerStart_{}".format(index + 1), "Hospital/Gameplay")
        actor_count += 1
    return actor_count


def build_power_puzzle():
    """Place three stable, accessible fuse boxes and the local visual controller."""
    actor_count = 0
    placements = (
        ("Fuse_West", "west-wing fuse", (-1675.0, -500.0, 115.0), 0.0),
        ("Fuse_Central", "central-wing fuse", (-25.0, 500.0, 115.0), 180.0),
        ("Fuse_East", "east-wing fuse", (1675.0, -500.0, 115.0), 180.0),
    )

    for fuse_id, fuse_label, location, yaw in placements:
        actor = ACTOR_SUBSYSTEM.spawn_actor_from_class(
            FUSE_BOX_CLASS,
            unreal.Vector(*location),
            unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0),
        )
        if not actor:
            raise RuntimeError("Unable to spawn hospital fuse box: {}".format(fuse_id))
        actor.set_editor_property("fuse_id", unreal.Name(fuse_id))
        actor.set_editor_property("fuse_label", fuse_label)
        mark_generated(
            actor,
            "FuseBox_{}".format(fuse_id),
            "Hospital/Gameplay/PowerPuzzle",
            ["HospitalFuseBox"],
        )
        actor_count += 1

    controller = ACTOR_SUBSYSTEM.spawn_actor_from_class(
        POWER_CONTROLLER_CLASS,
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0),
    )
    if not controller:
        raise RuntimeError("Unable to spawn hospital power controller")
    mark_generated(
        controller,
        "HospitalPowerController",
        "Hospital/Gameplay/PowerPuzzle",
    )
    return actor_count + 1


def build_navigation():
    # A single continuous hidden foundation gives Recast reliable geometry.
    # The visible hospital floor remains unchanged and sits at the same height.
    nav_floor = spawn_mesh(
        "navigation_floor",
        (0.0, 0.0, 5.0),
        label="HospitalNavigationFloor",
        folder="Hospital/Gameplay/Navigation",
    )
    nav_floor.set_actor_scale3d(unreal.Vector(34.0, 26.0, 0.1))
    nav_floor.set_actor_hidden_in_game(True)

    actor = ACTOR_SUBSYSTEM.spawn_actor_from_class(
        unreal.NavMeshBoundsVolume,
        unreal.Vector(0.0, 0.0, 130.0),
        unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0),
    )
    actor.set_actor_scale3d(unreal.Vector(18.0, 14.0, 2.0))
    mark_generated(actor, "HospitalNavMesh", "Hospital/Gameplay")
    return 2


def build_navigation_data(world):
    """Generate and embed Recast navigation data after all geometry exists."""
    unreal.log("HOSPITAL_STAGE|NAVIGATION_BUILD_BEGIN")
    unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")

    if unreal.NavigationSystemV1.is_navigation_being_built(world):
        raise RuntimeError("Hospital navigation build did not finish")

    nav_meshes = [
        actor
        for actor in ACTOR_SUBSYSTEM.get_all_level_actors()
        if isinstance(actor, unreal.RecastNavMesh)
    ]
    if not nav_meshes:
        raise RuntimeError("Hospital navigation build produced no RecastNavMesh")

    # The hospital is generated and may gain runtime puzzle/AI props. Dynamic
    # generation prevents stale baked tiles and the editor's "needs rebuild"
    # warning while keeping navigation authoritative on the host.
    for nav_mesh in nav_meshes:
        nav_mesh.set_editor_property(
            "runtime_generation",
            unreal.RuntimeGenerationType.DYNAMIC,
        )

    unreal.log(
        "HOSPITAL_NAVIGATION_COMPLETE|NAV_MESHES={}".format(len(nav_meshes))
    )


def main():
    world = prepare_level()
    unreal.log("HOSPITAL_STAGE|BUILD_BEGIN")

    total_actors = 0
    total_actors += build_floor_and_ceiling()
    total_actors += build_walls()
    total_actors += build_room_props()
    total_actors += build_lighting()
    total_actors += build_post_process()
    total_actors += build_player_starts()
    total_actors += build_power_puzzle()
    total_actors += build_navigation()
    build_navigation_data(world)

    LEVEL_EDITOR_SUBSYSTEM.save_current_level()
    unreal.EditorAssetLibrary.save_asset(MAP_PATH, only_if_is_dirty=False)
    unreal.log("HOSPITAL_MAP_COMPLETE|ACTORS={}|MAP={}".format(total_actors, MAP_PATH))


main()
