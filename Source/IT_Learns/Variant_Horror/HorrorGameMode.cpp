// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Horror/HorrorGameMode.h"

#include "IT_Learns.h"
#include "Variant_Horror/HorrorGameState.h"

AHorrorGameMode::AHorrorGameMode()
{
	GameStateClass = AHorrorGameState::StaticClass();
}

void AHorrorGameMode::BeginPlay()
{
	Super::BeginPlay();

	AHorrorGameState* HorrorGameState = GetGameState<AHorrorGameState>();
	if (!HorrorGameState || !HorrorGameState->GetObjectives().IsEmpty())
	{
		return;
	}

	HorrorGameState->AddObjective(
		TEXT("RestoreEmergencyPower"),
		NSLOCTEXT("HorrorObjectives", "RestorePowerTitle", "Restore emergency power"),
		NSLOCTEXT("HorrorObjectives", "RestorePowerDescription", "Locate and reset the three emergency fuse boxes."),
		3,
		false
	);

	HorrorGameState->AddObjective(
		TEXT("ReachRestrictedWard"),
		NSLOCTEXT("HorrorObjectives", "RestrictedWardTitle", "Reach the restricted ward"),
		NSLOCTEXT("HorrorObjectives", "RestrictedWardDescription", "Find a secure route deeper into the hospital."),
		0,
		false
	);
	HorrorGameState->SetObjectiveStatus(TEXT("ReachRestrictedWard"), EHorrorObjectiveStatus::Inactive);

	UE_LOG(LogIT_Learns, Display, TEXT("Initialized %d shared hospital objectives."), HorrorGameState->GetObjectives().Num());
}
