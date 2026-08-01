import math

import unreal


MAP_PATH = "/Game/Hospital/Maps/Lvl_Hospital"
EXPECTED_FUSE_IDS = {"Fuse_West", "Fuse_Central", "Fuse_East"}
INTERACTION_POINTS = {
    "Fuse_West": (-1400.0, -500.0, 100.0),
    "Fuse_Central": (-300.0, 500.0, 100.0),
    "Fuse_East": (1400.0, -500.0, 100.0),
}


level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if not level_subsystem.load_level(MAP_PATH):
    raise RuntimeError("Unable to load hospital map: {}".format(MAP_PATH))

world = editor_subsystem.get_editor_world()
fuse_class = unreal.load_class(None, "/Script/IT_Learns.HorrorFuseBox")
controller_class = unreal.load_class(None, "/Script/IT_Learns.HorrorPowerController")
if not fuse_class or not controller_class:
    raise RuntimeError("Hospital power-puzzle runtime classes are unavailable")

actors = actor_subsystem.get_all_level_actors()
fuse_actors = [actor for actor in actors if actor.get_class() == fuse_class]
controllers = [actor for actor in actors if actor.get_class() == controller_class]

found_ids = []
fuses_by_id = {}
for fuse in fuse_actors:
    fuse_id = str(fuse.get_editor_property("fuse_id"))
    found_ids.append(fuse_id)
    fuses_by_id[fuse_id] = fuse
    tags = {str(tag) for tag in fuse.get_editor_property("tags")}
    if "HospitalGenerated" not in tags or "HospitalFuseBox" not in tags:
        raise RuntimeError("Fuse {} is missing required generated/gameplay tags".format(fuse_id))
    if fuse.get_editor_property("activated"):
        raise RuntimeError("Fuse {} was saved in the activated state".format(fuse_id))

    unreal.log(
        "HOSPITAL_FUSE_CHECK|ID={}|LOCATION={}|ROTATION={}".format(
            fuse_id,
            fuse.get_actor_location(),
            fuse.get_actor_rotation(),
        )
    )

if len(fuse_actors) != 3 or set(found_ids) != EXPECTED_FUSE_IDS or len(set(found_ids)) != 3:
    raise RuntimeError(
        "Hospital fuse validation failed. Count={} IDs={}".format(
            len(fuse_actors),
            sorted(found_ids),
        )
    )

if len(controllers) != 1:
    raise RuntimeError("Expected one hospital power controller, found {}".format(len(controllers)))

mains_lights = [
    actor
    for actor in actors
    if unreal.Name("HospitalMainsLight") in actor.get_editor_property("tags")
]
emergency_lights = [
    actor
    for actor in actors
    if unreal.Name("HospitalEmergencyLight") in actor.get_editor_property("tags")
]
power_post_process = [
    actor
    for actor in actors
    if unreal.Name("HospitalPowerPostProcess") in actor.get_editor_property("tags")
]
if not mains_lights or not emergency_lights or len(power_post_process) != 1:
    raise RuntimeError(
        "Power targets invalid. Mains={} Emergency={} PostProcess={}".format(
            len(mains_lights),
            len(emergency_lights),
            len(power_post_process),
        )
    )

nav_meshes = [actor for actor in actors if isinstance(actor, unreal.RecastNavMesh)]
nav_volumes = [actor for actor in actors if isinstance(actor, unreal.NavMeshBoundsVolume)]
if not nav_meshes or not nav_volumes:
    raise RuntimeError("Hospital map is missing runtime navigation actors")
for nav_mesh in nav_meshes:
    if nav_mesh.get_editor_property("runtime_generation") != unreal.RuntimeGenerationType.DYNAMIC:
        raise RuntimeError("Hospital RecastNavMesh must generate dynamically at runtime")

for fuse_id, coordinates in INTERACTION_POINTS.items():
    location = fuses_by_id[fuse_id].get_actor_location()
    distance = math.sqrt(
        (location.x - coordinates[0]) ** 2
        + (location.y - coordinates[1]) ** 2
        + (location.z - coordinates[2]) ** 2
    )
    if distance > 325.0:
        raise RuntimeError(
            "Interaction position for {} is too far away: {:.1f} cm".format(
                fuse_id,
                distance,
            )
        )
    unreal.log(
        "HOSPITAL_FUSE_INTERACTION_CHECK|ID={}|DISTANCE_CM={:.1f}".format(
            fuse_id,
            distance,
        )
    )

unreal.log(
    "HOSPITAL_POWER_PUZZLE_VALIDATION_COMPLETE|FUSES={}|MAINS={}|EMERGENCY={}".format(
        len(fuse_actors),
        len(mains_lights),
        len(emergency_lights),
    )
)
