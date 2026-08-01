// Copyright Epic Games, Inc. All Rights Reserved.

#include "Variant_Horror/HorrorPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HorrorCharacter.h"
#include "HorrorUI.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "IT_Learns.h"
#include "IT_LearnsCameraManager.h"
#include "UI/HorrorMenuSubsystem.h"
#include "Variant_Horror/HorrorInteractable.h"
#include "Variant_Horror/UI/HorrorObjectiveWidget.h"
#include "Widgets/Input/SVirtualJoystick.h"

AHorrorPlayerController::AHorrorPlayerController()
{
	PlayerCameraManagerClass = AIT_LearnsCameraManager::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
}

void AHorrorPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController())
	{
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(false);
		SetInputMode(InputMode);
		bShowMouseCursor = false;
		bEnableClickEvents = false;
		bEnableMouseOverEvents = false;
		ResetIgnoreMoveInput();
		ResetIgnoreLookInput();
		InitializeGameplayHUD();
	}

	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			MobileControlsWidget->AddToPlayerScreen(0);
		}
		else
		{
			UE_LOG(LogIT_Learns, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}
}

void AHorrorPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);
	InitializeGameplayHUD();
}

void AHorrorPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CurrentInteractionTarget.Reset();

	if (HorrorUI)
	{
		HorrorUI->SetupCharacter(nullptr);
		HorrorUI->RemoveFromParent();
		HorrorUI = nullptr;
	}

	if (ObjectiveHUD)
	{
		ObjectiveHUD->RemoveFromParent();
		ObjectiveHUD = nullptr;
	}

	if (MobileControlsWidget)
	{
		MobileControlsWidget->RemoveFromParent();
		MobileControlsWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AHorrorPlayerController::OnPossess(APawn* aPawn)
{
	Super::OnPossess(aPawn);

	if (!IsLocalPlayerController())
	{
		return;
	}

	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(false);
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();

	InitializeGameplayHUD();
}

void AHorrorPlayerController::InitializeGameplayHUD()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (!HorrorUI && HorrorUIClass)
	{
		HorrorUI = CreateWidget<UHorrorUI>(this, HorrorUIClass);
		if (HorrorUI)
		{
			HorrorUI->AddToPlayerScreen(0);
		}
	}

	if (HorrorUI)
	{
		HorrorUI->SetupCharacter(Cast<AHorrorCharacter>(GetPawn()));
	}

	if (!ObjectiveHUD)
	{
		ObjectiveHUD = CreateWidget<UHorrorObjectiveWidget>(this, UHorrorObjectiveWidget::StaticClass());
		if (ObjectiveHUD)
		{
			ObjectiveHUD->AddToPlayerScreen(10);
			UE_LOG(LogIT_Learns, Display, TEXT("Mission objective HUD initialized."));
		}
	}
}

void AHorrorPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AHorrorPlayerController::TogglePauseMenu);
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AHorrorPlayerController::TryInteract);
	InputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, this, &AHorrorPlayerController::TryInteract);

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			if (CurrentContext)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}
		}

		if (!ShouldUseTouchControls())
		{
			for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
			{
				if (CurrentContext)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

void AHorrorPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (IsLocalPlayerController())
	{
		UpdateInteractionTarget();
	}
}

void AHorrorPlayerController::UpdateInteractionTarget()
{
	AActor* NewTarget = nullptr;
	FText Prompt;
	bool bCanInteract = false;

	if (UWorld* World = GetWorld())
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		GetPlayerViewPoint(ViewLocation, ViewRotation);

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HorrorInteractionTrace), false, GetPawn());
		FHitResult Hit;
		const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * InteractionDistance;
		if (World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams))
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && HitActor->GetClass()->ImplementsInterface(UHorrorInteractable::StaticClass()))
			{
				NewTarget = HitActor;
				bCanInteract = IHorrorInteractable::Execute_CanInteract(HitActor, GetPawn());
				Prompt = IHorrorInteractable::Execute_GetInteractionPrompt(HitActor, GetPawn());
			}
		}
	}

	CurrentInteractionTarget = NewTarget;
	if (ObjectiveHUD)
	{
		ObjectiveHUD->SetInteractionPrompt(Prompt, bCanInteract);
	}
}

void AHorrorPlayerController::TryInteract()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	// Refresh immediately so a stale look-at target can never be submitted.
	UpdateInteractionTarget();
	AActor* Target = CurrentInteractionTarget.Get();
	if (!Target || !Target->GetClass()->ImplementsInterface(UHorrorInteractable::StaticClass())
		|| !IHorrorInteractable::Execute_CanInteract(Target, GetPawn()))
	{
		return;
	}

	ServerTryInteract(Target);
}

void AHorrorPlayerController::ServerTryInteract_Implementation(AActor* Target)
{
	if (!IsServerInteractionValid(Target)
		|| !IHorrorInteractable::Execute_CanInteract(Target, GetPawn()))
	{
		return;
	}

	IHorrorInteractable::Execute_Interact(Target, GetPawn());
}

bool AHorrorPlayerController::IsServerInteractionValid(AActor* Target) const
{
	APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!Target || !ControlledPawn || !World
		|| Target->GetWorld() != World
		|| !Target->GetClass()->ImplementsInterface(UHorrorInteractable::StaticClass()))
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	ControlledPawn->GetActorEyesViewPoint(ViewLocation, ViewRotation);
	const FVector TargetLocation = Target->GetActorLocation();
	const float ValidationDistance = InteractionDistance + 100.0f;
	if (FVector::DistSquared(ViewLocation, TargetLocation) > FMath::Square(ValidationDistance))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HorrorServerInteractionTrace), false, ControlledPawn);
	FHitResult Hit;
	return World->LineTraceSingleByChannel(Hit, ViewLocation, TargetLocation, ECC_Visibility, QueryParams)
		&& Hit.GetActor() == Target;
}

void AHorrorPlayerController::TogglePauseMenu()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UHorrorMenuSubsystem* MenuSubsystem = GameInstance->GetSubsystem<UHorrorMenuSubsystem>())
		{
			MenuSubsystem->TogglePauseMenu();
		}
	}
}

bool AHorrorPlayerController::ShouldUseTouchControls() const
{
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
