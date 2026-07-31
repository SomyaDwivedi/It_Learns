// Copyright Epic Games, Inc. All Rights Reserved.

#include "Variant_Horror/HorrorPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "HorrorCharacter.h"
#include "HorrorUI.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "IT_Learns.h"
#include "IT_LearnsCameraManager.h"
#include "UI/HorrorMenuSubsystem.h"
#include "Widgets/Input/SVirtualJoystick.h"

AHorrorPlayerController::AHorrorPlayerController()
{
	PlayerCameraManagerClass = AIT_LearnsCameraManager::StaticClass();
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

	if (AHorrorCharacter* HorrorCharacter = Cast<AHorrorCharacter>(aPawn))
	{
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
			HorrorUI->SetupCharacter(HorrorCharacter);
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