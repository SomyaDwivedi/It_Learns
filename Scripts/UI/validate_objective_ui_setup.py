import unreal


GAME_MODE_CLASS_PATH = (
    "/Game/Variant_Horror/Blueprints/BP_HorrorGameMode.BP_HorrorGameMode_C"
)
EXPECTED_GAME_STATE_PATH = "/Script/IT_Learns.HorrorGameState"
ASSETS_TO_RESAVE = (
    "/Game/Variant_Horror/Blueprints/BP_HorrorGameMode",
    "/Game/Variant_Horror/Blueprints/BP_HorrorPlayerController",
    "/Game/Variant_Horror/UI/UI_Horror",
)


game_mode_class = unreal.load_class(None, GAME_MODE_CLASS_PATH)
if not game_mode_class:
    raise RuntimeError("Unable to load {}".format(GAME_MODE_CLASS_PATH))

game_mode_default = unreal.get_default_object(game_mode_class)
game_state_class = game_mode_default.get_editor_property("game_state_class")
game_state_path = game_state_class.get_path_name() if game_state_class else "None"
unreal.log(
    "OBJECTIVE_UI_GAME_STATE_CHECK|ACTUAL={}|EXPECTED={}".format(
        game_state_path,
        EXPECTED_GAME_STATE_PATH,
    )
)
if game_state_path != EXPECTED_GAME_STATE_PATH:
    raise RuntimeError(
        "BP_HorrorGameMode uses {}, expected {}".format(
            game_state_path,
            EXPECTED_GAME_STATE_PATH,
        )
    )

for asset_path in ASSETS_TO_RESAVE:
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        raise RuntimeError("Unable to load {}".format(asset_path))
    if not unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False):
        raise RuntimeError("Unable to resave {}".format(asset_path))
    unreal.log("OBJECTIVE_UI_RESAVED|{}".format(asset_path))

unreal.log("OBJECTIVE_UI_SETUP_VALIDATION_COMPLETE")
