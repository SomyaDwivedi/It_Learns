import unreal


MAP_PATH = "/Game/Hospital/Maps/Lvl_Hospital"
ROOM_X_POSITIONS = (-1200, -400, 400, 1200)
EXPECTED_LABELS = {
    "{}_{}".format(side, x)
    for side in ("CorridorNorth", "CorridorSouth")
    for x in ROOM_X_POSITIONS
}


level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if not level_subsystem.load_level(MAP_PATH):
    raise RuntimeError("Unable to load hospital map: {}".format(MAP_PATH))

found = set()
invalid = []

for actor in actor_subsystem.get_all_level_actors():
    label = actor.get_actor_label()
    if label not in EXPECTED_LABELS:
        continue

    found.add(label)
    component = actor.get_editor_property("static_mesh_component")
    profile = component.get_collision_profile_name()
    enabled = component.get_collision_enabled()
    unreal.log(
        "HOSPITAL_DOOR_CHECK|LABEL={}|PROFILE={}|ENABLED={}".format(
            label,
            profile,
            enabled,
        )
    )
    if profile != unreal.Name("NoCollision"):
        invalid.append("{} uses {}".format(label, profile))

missing = EXPECTED_LABELS - found
if missing or invalid:
    raise RuntimeError(
        "Hospital doorway validation failed. Missing: {} Invalid: {}".format(
            sorted(missing),
            invalid,
        )
    )

unreal.log("HOSPITAL_DOOR_VALIDATION_COMPLETE|COUNT={}".format(len(found)))
