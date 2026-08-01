#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HorrorPowerController.generated.h"

class AGameStateBase;
class AHorrorGameState;
class APostProcessVolume;
class UPointLightComponent;

/** Applies the replicated hospital power state to tagged lights and exposure on every machine. */
UCLASS()
class IT_LEARNS_API AHorrorPowerController : public AActor
{
    GENERATED_BODY()

public:
    AHorrorPowerController();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, Category="Horror|Power", meta=(ClampMin="0.0", ClampMax="1.0"))
    float OutageMainsMultiplier = 0.12f;

    UPROPERTY(EditAnywhere, Category="Horror|Power", meta=(ClampMin="0.0", ClampMax="1.0"))
    float RestoredEmergencyMultiplier = 0.04f;

    UPROPERTY(EditAnywhere, Category="Horror|Power")
    float RestoredExposureBias = -1.5f;

private:
    void CachePowerTargets();
    void HandleGameStateSet(AGameStateBase* NewGameState);
    void TryBindGameState();
    void ApplyPowerState();

    TWeakObjectPtr<AHorrorGameState> BoundGameState;
    TMap<TWeakObjectPtr<UPointLightComponent>, float> MainsLightIntensities;
    TMap<TWeakObjectPtr<UPointLightComponent>, float> EmergencyLightIntensities;
    TArray<TWeakObjectPtr<APostProcessVolume>> PowerPostProcessVolumes;
    bool bHasAppliedState = false;
    bool bLastAppliedPowerState = false;
};
