#include "Variant_Horror/HorrorGameState.h"

#include "Net/UnrealNetwork.h"

float FHorrorObjectiveState::GetProgressFraction() const
{
    if (TargetProgress <= 0)
    {
        return Status == EHorrorObjectiveStatus::Completed ? 1.0f : 0.0f;
    }

    return FMath::Clamp(static_cast<float>(CurrentProgress) / static_cast<float>(TargetProgress), 0.0f, 1.0f);
}

FText FHorrorObjectiveState::GetProgressText() const
{
    if (TargetProgress <= 0)
    {
        return FText::GetEmpty();
    }

    return FText::Format(
        NSLOCTEXT("HorrorObjectives", "ObjectiveProgressFormat", "{0} / {1}"),
        FText::AsNumber(FMath::Clamp(CurrentProgress, 0, TargetProgress)),
        FText::AsNumber(TargetProgress)
    );
}

bool FHorrorObjectiveState::IsVisibleInHUD() const
{
    return Status == EHorrorObjectiveStatus::Active
        || Status == EHorrorObjectiveStatus::Completed
        || Status == EHorrorObjectiveStatus::Failed;
}

bool FHorrorObjectiveState::ApplyProgress(int32 NewProgress)
{
    if (TargetProgress <= 0)
    {
        return false;
    }

    const int32 ClampedProgress = FMath::Clamp(NewProgress, 0, TargetProgress);
    const EHorrorObjectiveStatus NewStatus = ClampedProgress >= TargetProgress
        ? EHorrorObjectiveStatus::Completed
        : EHorrorObjectiveStatus::Active;

    if (CurrentProgress == ClampedProgress && Status == NewStatus)
    {
        return false;
    }

    CurrentProgress = ClampedProgress;
    Status = NewStatus;
    return true;
}

bool FHorrorFusePuzzleState::TryActivateFuse(FName FuseId)
{
    if (FuseId.IsNone() || bEmergencyPowerRestored || ActivatedFuseIds.Contains(FuseId))
    {
        return false;
    }

    ActivatedFuseIds.Add(FuseId);
    bEmergencyPowerRestored = ActivatedFuseIds.Num() >= FMath::Max(1, RequiredFuseCount);
    return true;
}

bool FHorrorFusePuzzleState::IsFuseActivated(FName FuseId) const
{
    return !FuseId.IsNone() && ActivatedFuseIds.Contains(FuseId);
}

AHorrorGameState::AHorrorGameState()
{
    bReplicates = true;
}

void AHorrorGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AHorrorGameState, Objectives);
    DOREPLIFETIME(AHorrorGameState, FusePuzzleState);
}

bool AHorrorGameState::AddObjective(
    FName ObjectiveId,
    const FText& Title,
    const FText& Description,
    int32 TargetProgress,
    bool bOptional
)
{
    if (!HasAuthority() || ObjectiveId.IsNone() || Title.IsEmpty())
    {
        return false;
    }

    const int32 ExistingIndex = FindObjectiveIndex(ObjectiveId);
    FHorrorObjectiveState* Objective = ExistingIndex == INDEX_NONE
        ? &Objectives.AddDefaulted_GetRef()
        : &Objectives[ExistingIndex];

    Objective->ObjectiveId = ObjectiveId;
    Objective->Title = Title;
    Objective->Description = Description;
    Objective->TargetProgress = FMath::Max(0, TargetProgress);
    Objective->CurrentProgress = FMath::Clamp(Objective->CurrentProgress, 0, Objective->TargetProgress);
    Objective->Status = EHorrorObjectiveStatus::Active;
    Objective->bOptional = bOptional;

    NotifyObjectivesChanged();
    return true;
}

bool AHorrorGameState::SetObjectiveProgress(FName ObjectiveId, int32 NewProgress)
{
    if (!HasAuthority())
    {
        return false;
    }

    const int32 ObjectiveIndex = FindObjectiveIndex(ObjectiveId);
    if (!Objectives.IsValidIndex(ObjectiveIndex) || !Objectives[ObjectiveIndex].ApplyProgress(NewProgress))
    {
        return false;
    }

    NotifyObjectivesChanged();
    return true;
}

bool AHorrorGameState::IncrementObjectiveProgress(FName ObjectiveId, int32 ProgressDelta)
{
    const int32 ObjectiveIndex = FindObjectiveIndex(ObjectiveId);
    if (!HasAuthority() || !Objectives.IsValidIndex(ObjectiveIndex))
    {
        return false;
    }

    return SetObjectiveProgress(ObjectiveId, Objectives[ObjectiveIndex].CurrentProgress + ProgressDelta);
}

bool AHorrorGameState::SetObjectiveStatus(FName ObjectiveId, EHorrorObjectiveStatus NewStatus)
{
    if (!HasAuthority())
    {
        return false;
    }

    const int32 ObjectiveIndex = FindObjectiveIndex(ObjectiveId);
    if (!Objectives.IsValidIndex(ObjectiveIndex) || Objectives[ObjectiveIndex].Status == NewStatus)
    {
        return false;
    }

    Objectives[ObjectiveIndex].Status = NewStatus;
    if (NewStatus == EHorrorObjectiveStatus::Completed && Objectives[ObjectiveIndex].TargetProgress > 0)
    {
        Objectives[ObjectiveIndex].CurrentProgress = Objectives[ObjectiveIndex].TargetProgress;
    }

    NotifyObjectivesChanged();
    return true;
}

bool AHorrorGameState::FindObjective(FName ObjectiveId, FHorrorObjectiveState& OutObjective) const
{
    const int32 ObjectiveIndex = FindObjectiveIndex(ObjectiveId);
    if (!Objectives.IsValidIndex(ObjectiveIndex))
    {
        return false;
    }

    OutObjective = Objectives[ObjectiveIndex];
    return true;
}

bool AHorrorGameState::RegisterFuseActivation(FName FuseId)
{
    if (!HasAuthority() || !IsKnownHospitalFuse(FuseId) || FusePuzzleState.bEmergencyPowerRestored)
    {
        return false;
    }

    FHorrorObjectiveState PowerObjective;
    if (!FindObjective(TEXT("RestoreEmergencyPower"), PowerObjective)
        || PowerObjective.Status != EHorrorObjectiveStatus::Active
        || PowerObjective.TargetProgress != FusePuzzleState.RequiredFuseCount)
    {
        return false;
    }

    FHorrorFusePuzzleState NextState = FusePuzzleState;
    if (!NextState.TryActivateFuse(FuseId))
    {
        return false;
    }

    if (!SetObjectiveProgress(TEXT("RestoreEmergencyPower"), NextState.GetActivatedFuseCount()))
    {
        return false;
    }

    FusePuzzleState = MoveTemp(NextState);
    if (FusePuzzleState.bEmergencyPowerRestored)
    {
        SetObjectiveStatus(TEXT("ReachRestrictedWard"), EHorrorObjectiveStatus::Active);
    }

    NotifyFusePuzzleChanged();
    return true;
}

bool AHorrorGameState::IsFuseActivated(FName FuseId) const
{
    return FusePuzzleState.IsFuseActivated(FuseId);
}

void AHorrorGameState::OnRep_Objectives()
{
    ObjectivesChanged.Broadcast();
}

void AHorrorGameState::OnRep_FusePuzzleState()
{
    FusePuzzleChanged.Broadcast();
}

int32 AHorrorGameState::FindObjectiveIndex(FName ObjectiveId) const
{
    return Objectives.IndexOfByPredicate(
        [ObjectiveId](const FHorrorObjectiveState& Objective)
        {
            return Objective.ObjectiveId == ObjectiveId;
        }
    );
}

bool AHorrorGameState::IsKnownHospitalFuse(FName FuseId)
{
    return FuseId == TEXT("Fuse_West")
        || FuseId == TEXT("Fuse_Central")
        || FuseId == TEXT("Fuse_East");
}

void AHorrorGameState::NotifyObjectivesChanged()
{
    ObjectivesChanged.Broadcast();
    ForceNetUpdate();
}

void AHorrorGameState::NotifyFusePuzzleChanged()
{
    FusePuzzleChanged.Broadcast();
    ForceNetUpdate();
}
