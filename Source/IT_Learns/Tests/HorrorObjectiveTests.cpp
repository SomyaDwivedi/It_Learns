#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Variant_Horror/HorrorGameState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHorrorObjectiveProgressTest,
    "ITLearns.Horror.UI.Objectives.Progress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FHorrorObjectiveProgressTest::RunTest(const FString& Parameters)
{
    FHorrorObjectiveState Objective;
    Objective.ObjectiveId = TEXT("RestoreEmergencyPower");
    Objective.Status = EHorrorObjectiveStatus::Active;
    Objective.TargetProgress = 3;

    TestTrue(TEXT("First progress update changes the objective"), Objective.ApplyProgress(1));
    TestEqual(TEXT("Progress is stored"), Objective.CurrentProgress, 1);
    TestEqual(TEXT("Progress text is formatted"), Objective.GetProgressText().ToString(), FString(TEXT("1 / 3")));
    TestTrue(TEXT("Progress fraction is correct"), FMath::IsNearlyEqual(Objective.GetProgressFraction(), 1.0f / 3.0f));
    TestFalse(TEXT("Repeating the same progress is ignored"), Objective.ApplyProgress(1));

    TestTrue(TEXT("Progress clamps at the target"), Objective.ApplyProgress(99));
    TestEqual(TEXT("Clamped progress reaches target"), Objective.CurrentProgress, 3);
    TestEqual(
        TEXT("Reaching target completes objective"),
        static_cast<uint8>(Objective.Status),
        static_cast<uint8>(EHorrorObjectiveStatus::Completed)
    );
    TestTrue(TEXT("Completed progress is full"), FMath::IsNearlyEqual(Objective.GetProgressFraction(), 1.0f));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHorrorObjectiveVisibilityTest,
    "ITLearns.Horror.UI.Objectives.Visibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FHorrorObjectiveVisibilityTest::RunTest(const FString& Parameters)
{
    FHorrorObjectiveState Objective;
    Objective.Status = EHorrorObjectiveStatus::Inactive;
    TestFalse(TEXT("Inactive objectives stay hidden"), Objective.IsVisibleInHUD());

    Objective.Status = EHorrorObjectiveStatus::Active;
    TestTrue(TEXT("Active objectives are displayed"), Objective.IsVisibleInHUD());

    Objective.Status = EHorrorObjectiveStatus::Completed;
    TestTrue(TEXT("Completed objectives retain completion feedback"), Objective.IsVisibleInHUD());

    Objective.Status = EHorrorObjectiveStatus::Failed;
    TestTrue(TEXT("Failed objectives display their result"), Objective.IsVisibleInHUD());

    Objective.TargetProgress = 0;
    TestTrue(TEXT("Non-numeric objectives have no progress label"), Objective.GetProgressText().IsEmpty());
    TestFalse(TEXT("Non-numeric objectives reject numeric progress"), Objective.ApplyProgress(1));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHorrorFusePuzzleStateTest,
    "ITLearns.Horror.Puzzle.Fuses.UniqueCompletion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FHorrorFusePuzzleStateTest::RunTest(const FString& Parameters)
{
    FHorrorFusePuzzleState Puzzle;

    TestFalse(TEXT("An empty fuse ID is rejected"), Puzzle.TryActivateFuse(NAME_None));
    TestEqual(TEXT("Rejected input does not change progress"), Puzzle.GetActivatedFuseCount(), 0);

    TestTrue(TEXT("The west fuse activates"), Puzzle.TryActivateFuse(TEXT("Fuse_West")));
    TestEqual(TEXT("First unique fuse counts once"), Puzzle.GetActivatedFuseCount(), 1);
    TestTrue(TEXT("The west fuse is recorded"), Puzzle.IsFuseActivated(TEXT("Fuse_West")));
    TestFalse(TEXT("One fuse does not restore power"), Puzzle.bEmergencyPowerRestored);

    TestFalse(TEXT("A repeated west fuse is rejected"), Puzzle.TryActivateFuse(TEXT("Fuse_West")));
    TestEqual(TEXT("A repeated fuse cannot overcount"), Puzzle.GetActivatedFuseCount(), 1);

    TestTrue(TEXT("The central fuse activates"), Puzzle.TryActivateFuse(TEXT("Fuse_Central")));
    TestEqual(TEXT("Second unique fuse advances progress"), Puzzle.GetActivatedFuseCount(), 2);
    TestFalse(TEXT("Two fuses do not restore power"), Puzzle.bEmergencyPowerRestored);

    TestTrue(TEXT("The east fuse activates"), Puzzle.TryActivateFuse(TEXT("Fuse_East")));
    TestEqual(TEXT("Three unique fuses complete the ledger"), Puzzle.GetActivatedFuseCount(), 3);
    TestTrue(TEXT("The third fuse restores emergency power"), Puzzle.bEmergencyPowerRestored);

    TestFalse(TEXT("Post-completion activations are rejected"), Puzzle.TryActivateFuse(TEXT("Fuse_Extra")));
    TestEqual(TEXT("Completed progress remains exactly three"), Puzzle.GetActivatedFuseCount(), 3);

    return true;
}

#endif
