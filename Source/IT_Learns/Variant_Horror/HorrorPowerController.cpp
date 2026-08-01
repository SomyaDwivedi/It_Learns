#include "Variant_Horror/HorrorPowerController.h"

#include "Components/PointLightComponent.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "IT_Learns.h"
#include "Variant_Horror/HorrorGameState.h"

namespace
{
    const FName MainsLightTag(TEXT("HospitalMainsLight"));
    const FName EmergencyLightTag(TEXT("HospitalEmergencyLight"));
    const FName PowerPostProcessTag(TEXT("HospitalPowerPostProcess"));
}

AHorrorPowerController::AHorrorPowerController()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AHorrorPowerController::BeginPlay()
{
    Super::BeginPlay();

    CachePowerTargets();
    if (UWorld* World = GetWorld())
    {
        World->GameStateSetEvent.RemoveAll(this);
        World->GameStateSetEvent.AddUObject(this, &AHorrorPowerController::HandleGameStateSet);
    }
    TryBindGameState();
}

void AHorrorPowerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (BoundGameState.IsValid())
    {
        BoundGameState->OnFusePuzzleChanged().RemoveAll(this);
    }
    BoundGameState.Reset();

    if (UWorld* World = GetWorld())
    {
        World->GameStateSetEvent.RemoveAll(this);
    }

    Super::EndPlay(EndPlayReason);
}

void AHorrorPowerController::CachePowerTargets()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (TActorIterator<APointLight> It(World); It; ++It)
    {
        UPointLightComponent* LightComponent = It->PointLightComponent;
        if (!LightComponent)
        {
            continue;
        }

        if (It->ActorHasTag(MainsLightTag))
        {
            MainsLightIntensities.Add(LightComponent, LightComponent->Intensity);
        }
        else if (It->ActorHasTag(EmergencyLightTag))
        {
            EmergencyLightIntensities.Add(LightComponent, LightComponent->Intensity);
        }
    }

    for (TActorIterator<APostProcessVolume> It(World); It; ++It)
    {
        if (It->ActorHasTag(PowerPostProcessTag))
        {
            PowerPostProcessVolumes.Add(*It);
        }
    }
}

void AHorrorPowerController::HandleGameStateSet(AGameStateBase* NewGameState)
{
    (void)NewGameState;
    TryBindGameState();
}

void AHorrorPowerController::TryBindGameState()
{
    AHorrorGameState* CurrentGameState = GetWorld() ? GetWorld()->GetGameState<AHorrorGameState>() : nullptr;
    if (CurrentGameState != BoundGameState.Get())
    {
        if (BoundGameState.IsValid())
        {
            BoundGameState->OnFusePuzzleChanged().RemoveAll(this);
        }

        BoundGameState = CurrentGameState;
        if (BoundGameState.IsValid())
        {
            BoundGameState->OnFusePuzzleChanged().AddUObject(this, &AHorrorPowerController::ApplyPowerState);
        }
    }

    ApplyPowerState();
}

void AHorrorPowerController::ApplyPowerState()
{
    const bool bPowerRestored = BoundGameState.IsValid() && BoundGameState->IsEmergencyPowerRestored();

    for (const TPair<TWeakObjectPtr<UPointLightComponent>, float>& Pair : MainsLightIntensities)
    {
        if (UPointLightComponent* Light = Pair.Key.Get())
        {
            Light->SetIntensity(Pair.Value * (bPowerRestored ? 1.0f : OutageMainsMultiplier));
        }
    }

    for (const TPair<TWeakObjectPtr<UPointLightComponent>, float>& Pair : EmergencyLightIntensities)
    {
        if (UPointLightComponent* Light = Pair.Key.Get())
        {
            Light->SetIntensity(Pair.Value * (bPowerRestored ? RestoredEmergencyMultiplier : 1.0f));
        }
    }

    for (const TWeakObjectPtr<APostProcessVolume>& VolumePtr : PowerPostProcessVolumes)
    {
        if (APostProcessVolume* Volume = VolumePtr.Get())
        {
            Volume->Settings.bOverride_AutoExposureBias = true;
            Volume->Settings.AutoExposureBias = bPowerRestored ? RestoredExposureBias : -3.0f;
        }
    }

    if (!bHasAppliedState || bPowerRestored != bLastAppliedPowerState)
    {
        UE_LOG(
            LogIT_Learns,
            Display,
            TEXT("Hospital emergency power state applied: %s."),
            bPowerRestored ? TEXT("RESTORED") : TEXT("OUTAGE")
        );
    }
    bHasAppliedState = true;
    bLastAppliedPowerState = bPowerRestored;
}
