// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HorrorPlayerController.generated.h"

class UInputMappingContext;
class UHorrorObjectiveWidget;
class UHorrorUI;

/**
 * Player Controller for a first person horror game.
 * Manages input mappings, HUD, and the local pause menu.
 */
UCLASS(abstract, config="Game")
class IT_LEARNS_API AHorrorPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, Category="Horror|UI")
	TSubclassOf<UHorrorUI> HorrorUIClass;

	UPROPERTY()
	TObjectPtr<UHorrorUI> HorrorUI;

	UPROPERTY(Transient)
	TObjectPtr<UHorrorObjectiveWidget> ObjectiveHUD;

public:
	AHorrorPlayerController();

protected:
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	// Keep this non-reflected to match the serialized layout used by
	// BP_HorrorPlayerController on the working Khushpreet branch.
	TObjectPtr<UUserWidget> MobileControlsWidget;

	UPROPERTY(EditAnywhere, Config, Category="Input|Touch Controls")
	bool bForceTouchControls = false;

	UPROPERTY(EditDefaultsOnly, Category="Horror|Interaction", meta=(ClampMin="100.0", Units="cm"))
	float InteractionDistance = 325.0f;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetPawn(APawn* InPawn) override;
	virtual void OnPossess(APawn* aPawn) override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	void TogglePauseMenu();
	void InitializeGameplayHUD();
	void UpdateInteractionTarget();
	void TryInteract();
	bool IsServerInteractionValid(AActor* Target) const;
	bool ShouldUseTouchControls() const;

	UFUNCTION(Server, Reliable)
	void ServerTryInteract(AActor* Target);

	TWeakObjectPtr<AActor> CurrentInteractionTarget;
};
