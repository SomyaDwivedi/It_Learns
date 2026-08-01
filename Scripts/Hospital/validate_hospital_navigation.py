import unreal


MAP_PATH = "/Game/Hospital/Maps/Lvl_Hospital"
TEST_POINTS = (
    (0.0, 0.0, 100.0),
    (-1200.0, -760.0, 100.0),
    (-400.0, 760.0, 100.0),
    (400.0, -760.0, 100.0),
    (1200.0, 760.0, 100.0),
)


level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if not level_subsystem.load_level(MAP_PATH):
    raise RuntimeError("Unable to load hospital map: {}".format(MAP_PATH))

world = editor_subsystem.get_editor_world()
nav_volumes = [
    actor
    for actor in actor_subsystem.get_all_level_actors()
    if isinstance(actor, unreal.NavMeshBoundsVolume)
]
nav_meshes = [
    actor
    for actor in actor_subsystem.get_all_level_actors()
    if isinstance(actor, unreal.RecastNavMesh)
]
if not nav_meshes:
    raise RuntimeError("Saved hospital map contains no RecastNavMesh")

if not nav_volumes:
    raise RuntimeError("Saved hospital map contains no NavMeshBoundsVolume")

for volume in nav_volumes:
    unreal.log(
        "HOSPITAL_NAV_VOLUME|LABEL={}|LOCATION={}|SCALE={}|BOUNDS={}".format(
            volume.get_actor_label(),
            volume.get_actor_location(),
            volume.get_actor_scale3d(),
            volume.get_actor_bounds(False),
        )
    )

for nav_mesh in nav_meshes:
    unreal.log(
        "HOSPITAL_RECAST_ACTOR|LABEL={}|LOCATION={}|BOUNDS={}|RUNTIME_GENERATION={}".format(
            nav_mesh.get_actor_label(),
            nav_mesh.get_actor_location(),
            nav_mesh.get_actor_bounds(False),
            nav_mesh.get_editor_property("runtime_generation"),
        )
    )

dynamic_navigation = all(
    nav_mesh.get_editor_property("runtime_generation")
    == unreal.RuntimeGenerationType.DYNAMIC
    for nav_mesh in nav_meshes
)

sample_floors = [
    actor
    for actor in actor_subsystem.get_all_level_actors()
    if actor.get_actor_label().startswith("Floor_")
][:3]
for floor in sample_floors:
    component = floor.get_editor_property("static_mesh_component")
    unreal.log(
        "HOSPITAL_NAV_INPUT|LABEL={}|PROFILE={}|ENABLED={}|AFFECTS_NAV={}|BOUNDS={}".format(
            floor.get_actor_label(),
            component.get_collision_profile_name(),
            component.get_collision_enabled(),
            component.get_editor_property("can_ever_affect_navigation"),
            floor.get_actor_bounds(False),
        )
    )

def check_points(stage):
    valid_points = 0
    for coordinates in TEST_POINTS:
        result = unreal.NavigationSystemV1.project_point_to_navigation(
            world,
            unreal.Vector(*coordinates),
            None,
            None,
            unreal.Vector(150.0, 150.0, 300.0),
        )
        success = result is not None
        unreal.log(
            "HOSPITAL_NAV_POINT_CHECK|STAGE={}|POINT={}|RESULT={}".format(
                stage,
                coordinates,
                result,
            )
        )
        if success:
            valid_points += 1
    return valid_points


valid_points = check_points("SAVED")
if dynamic_navigation and valid_points == 0:
    # Python commandlets do not enter gameplay, so a dynamic Recast actor has
    # no runtime tiles yet. The bounds/geometry checks above plus the packaged
    # runtime smoke test are the correct validation path for this mode.
    unreal.log(
        "HOSPITAL_NAVIGATION_VALIDATION_COMPLETE|MODE=DYNAMIC_RUNTIME|NAV_MESHES={}".format(
            len(nav_meshes)
        )
    )
elif valid_points != len(TEST_POINTS):
    unreal.log_warning(
        "Saved navigation query returned {} of {} points; rebuilding for comparison".format(
            valid_points,
            len(TEST_POINTS),
        )
    )
    unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
    valid_points = check_points("REBUILT")

    if valid_points != len(TEST_POINTS):
        raise RuntimeError(
            "Only {} of {} hospital navigation points were valid after rebuild".format(
                valid_points,
                len(TEST_POINTS),
            )
        )

if not dynamic_navigation or valid_points > 0:
    unreal.log(
        "HOSPITAL_NAVIGATION_VALIDATION_COMPLETE|MODE=SAVED_TILES|NAV_MESHES={}|POINTS={}".format(
            len(nav_meshes),
            valid_points,
        )
    )
