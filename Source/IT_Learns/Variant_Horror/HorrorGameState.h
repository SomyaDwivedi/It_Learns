#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "HorrorGameState.generated.h"

UENUM(BlueprintType)
enum class EHorrorObjectiveStatus : uint8
{
    Inactive,
    Active,
    Completed,
    Failed
};

USTRUCT(BlueprintType)
struct IT_LEARNS_API FHorrorObjectiveState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objective")
    FName ObjectiveId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objective")
    FText Title;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objective")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objective")
    int32 CurrentProgress = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objective")
    int32 TargetProgress = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objective")
    EHorrorObjectiveStatus Status = EHorrorObjectiveStatus::Inactive;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objective")
    bool bOptional = false;

    float GetProgressFraction() const;
    FText GetProgressText() const;
    bool IsVisibleInHUD() const;
    bool ApplyProgress(int32 NewProgress);
};

/** Server-owned state for the hospital's three-fuse power puzzle. */
USTRUCT(BlueprintType)
struct IT_LEARNS_API FHorrorFusePuzzleState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Horror|Puzzle")
    TArray<FName> ActivatedFuseIds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Horror|Puzzle")
    int32 RequiredFuseCount = 3;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Horror|Puzzle")
    bool bEmergencyPowerRestored = false;

    bool TryActivateFuse(FName FuseId);
    bool IsFuseActivated(FName FuseId) const;
    int32 GetActivatedFuseCount() const { return ActivatedFuseIds.Num(); }
};

DECLARE_MULTICAST_DELEGATE(FHorrorObjectivesChanged);
DECLARE_MULTICAST_DELEGATE(FHorrorFusePuzzleChanged);

/**
 * Server-owned, replicated mission state shared by every player in the session.
 */
UCLASS()
class IT_LEARNS_API AHorrorGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AHorrorGameState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Horror|Objectives")
    bool AddObjective(
        FName ObjectiveId,
        const FText& Title,
        const FText& Description,
        int32 TargetProgress,
        bool bOptional
    );

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Horror|Objectives")
    bool SetObjectiveProgress(FName ObjectiveId, int32 NewProgress);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Horror|Objectives")
    bool IncrementObjectiveProgress(FName ObjectiveId, int32 ProgressDelta);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Horror|Objectives")
    bool SetObjectiveStatus(FName ObjectiveId, EHorrorObjectiveStatus NewStatus);

    UFUNCTION(BlueprintPure, Category="Horror|Objectives")
    bool FindObjective(FName ObjectiveId, FHorrorObjectiveState& OutObjective) const;

    /** Registers one of the three known hospital fuse boxes exactly once. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Horror|Puzzle")
    bool RegisterFuseActivation(FName FuseId);

    UFUNCTION(BlueprintPure, Category="Horror|Puzzle")
    bool IsFuseActivated(FName FuseId) const;

    UFUNCTION(BlueprintPure, Category="Horror|Puzzle")
    bool IsEmergencyPowerRestored() const { return FusePuzzleState.bEmergencyPowerRestored; }

    UFUNCTION(BlueprintPure, Category="Horror|Puzzle")
    int32 GetActivatedFuseCount() const { return FusePuzzleState.GetActivatedFuseCount(); }

    const TArray<FHorrorObjectiveState>& GetObjectives() const { return Objectives; }
    FHorrorObjectivesChanged& OnObjectivesChanged() { return ObjectivesChanged; }
    FHorrorFusePuzzleChanged& OnFusePuzzleChanged() { return FusePuzzleChanged; }

protected:
    UPROPERTY(ReplicatedUsing=OnRep_Objectives, BlueprintReadOnly, Category="Horror|Objectives")
    TArray<FHorrorObjectiveState> Objectives;

    UPROPERTY(ReplicatedUsing=OnRep_FusePuzzleState, BlueprintReadOnly, Category="Horror|Puzzle")
    FHorrorFusePuzzleState FusePuzzleState;

    UFUNCTION()
    void OnRep_Objectives();

    UFUNCTION()
    void OnRep_FusePuzzleState();

private:
    int32 FindObjectiveIndex(FName ObjectiveId) const;
    static bool IsKnownHospitalFuse(FName FuseId);
    void NotifyObjectivesChanged();
    void NotifyFusePuzzleChanged();

    FHorrorObjectivesChanged ObjectivesChanged;
    FHorrorFusePuzzleChanged FusePuzzleChanged;
};
