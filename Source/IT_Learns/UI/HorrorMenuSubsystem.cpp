#include "UI/HorrorMenuSubsystem.h"

#include "AudioDevice.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "EOSLoginLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "TimerManager.h"
#include "UI/HorrorMenuWidget.h"
#include "UObject/UObjectGlobals.h"

namespace
{
    const TCHAR* MenuSettingsSection = TEXT("/Script/IT_Learns.HorrorMenuSettings");
    const FName StartupMapPath(TEXT("/Game/Maps/GameStartupMap"));
}

void UHorrorMenuSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    GConfig->GetFloat(MenuSettingsSection, TEXT("MasterVolume"), MasterVolume, GGameUserSettingsIni);
    GConfig->GetFloat(MenuSettingsSection, TEXT("LookSensitivity"), LookSensitivity, GGameUserSettingsIni);
    MasterVolume = FMath::Clamp(MasterVolume, 0.0f, 1.0f);
    LookSensitivity = FMath::Clamp(LookSensitivity, 0.2f, 2.5f);

    PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
        this,
        &UHorrorMenuSubsystem::HandlePostLoadMap
    );
}

void UHorrorMenuSubsystem::Deinitialize()
{
    if (PostLoadMapHandle.IsValid())
    {
        FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
        PostLoadMapHandle.Reset();
    }

    if (DestroySessionHandle.IsValid())
    {
        if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
        {
            if (IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface(); Sessions.IsValid())
            {
                Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);
            }
        }
        DestroySessionHandle.Reset();
    }

    HideActiveMenu();
    Super::Deinitialize();
}

void UHorrorMenuSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
    if (!LoadedWorld || !LoadedWorld->IsGameWorld() || LoadedWorld->GetGameInstance() != GetGameInstance())
    {
        return;
    }

    HideActiveMenu();
    ApplyRuntimePreferences();

    if (IsStartupMap(LoadedWorld))
    {
        bPendingMainMenu = true;
        LoadedWorld->GetTimerManager().SetTimerForNextTick(this, &UHorrorMenuSubsystem::ShowMainMenuNextTick);
    }
    else if (APlayerController* PlayerController = GetFirstLocalPlayerController())
    {
        FInputModeGameOnly InputMode;
        InputMode.SetConsumeCaptureMouseDown(false);
        PlayerController->SetInputMode(InputMode);
        PlayerController->bShowMouseCursor = false;
        PlayerController->ResetIgnoreLookInput();
        PlayerController->ResetIgnoreMoveInput();
    }
}

void UHorrorMenuSubsystem::ShowMainMenuNextTick()
{
    if (!bPendingMainMenu)
    {
        return;
    }

    bPendingMainMenu = false;
    ShowMainMenu();
}

void UHorrorMenuSubsystem::ShowMainMenu()
{
    UWorld* World = GetWorld();
    APlayerController* PlayerController = GetFirstLocalPlayerController();
    if (!World || !PlayerController || !IsStartupMap(World))
    {
        return;
    }

    UWidgetLayoutLibrary::RemoveAllWidgets(this);
    ActiveMenu = CreateWidget<UHorrorMenuWidget>(PlayerController, UHorrorMenuWidget::StaticClass());
    if (!ActiveMenu)
    {
        return;
    }

    ActiveMenu->Configure(this, EHorrorMenuContext::Main);
    ActiveMenu->AddToViewport(100);

    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(ActiveMenu->TakeWidget());
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PlayerController->SetInputMode(InputMode);
    PlayerController->bShowMouseCursor = true;
}

void UHorrorMenuSubsystem::TogglePauseMenu()
{
    UWorld* World = GetWorld();
    APlayerController* PlayerController = GetFirstLocalPlayerController();
    if (!World || !PlayerController || IsStartupMap(World))
    {
        return;
    }

    if (IsPauseMenuOpen())
    {
        ResumeGame();
        return;
    }

    ActiveMenu = CreateWidget<UHorrorMenuWidget>(PlayerController, UHorrorMenuWidget::StaticClass());
    if (!ActiveMenu)
    {
        return;
    }

    ActiveMenu->Configure(this, EHorrorMenuContext::Pause);
    ActiveMenu->AddToViewport(1000);

    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(ActiveMenu->TakeWidget());
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PlayerController->SetInputMode(InputMode);
    PlayerController->bShowMouseCursor = true;
    PlayerController->SetIgnoreLookInput(true);
    PlayerController->SetIgnoreMoveInput(true);

    if (World->GetNetMode() == NM_Standalone)
    {
        PlayerController->SetPause(true);
    }
}

void UHorrorMenuSubsystem::ResumeGame()
{
    APlayerController* PlayerController = GetFirstLocalPlayerController();
    if (!PlayerController)
    {
        HideActiveMenu();
        return;
    }

    PlayerController->SetPause(false);
    HideActiveMenu();

    FInputModeGameOnly InputMode;
    InputMode.SetConsumeCaptureMouseDown(false);
    PlayerController->SetInputMode(InputMode);
    PlayerController->bShowMouseCursor = false;
    PlayerController->ResetIgnoreLookInput();
    PlayerController->ResetIgnoreMoveInput();
}

void UHorrorMenuSubsystem::ReturnToMainMenu()
{
    if (APlayerController* PlayerController = GetFirstLocalPlayerController())
    {
        PlayerController->SetPause(false);
    }

    if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
    {
        if (IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface(); Sessions.IsValid() && Sessions->GetNamedSession(NAME_GameSession))
        {
            if (DestroySessionHandle.IsValid())
            {
                Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);
            }

            DestroySessionHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
                FOnDestroySessionCompleteDelegate::CreateUObject(this, &UHorrorMenuSubsystem::HandleDestroySessionComplete)
            );

            if (Sessions->DestroySession(NAME_GameSession))
            {
                return;
            }

            Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);
            DestroySessionHandle.Reset();
        }
    }

    OpenStartupMap();
}

void UHorrorMenuSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
    {
        if (IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface(); Sessions.IsValid() && DestroySessionHandle.IsValid())
        {
            Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);
        }
    }

    DestroySessionHandle.Reset();
    OpenStartupMap();
}

void UHorrorMenuSubsystem::OpenStartupMap()
{
    HideActiveMenu();
    UGameplayStatics::OpenLevel(this, StartupMapPath, true);
}

void UHorrorMenuSubsystem::QuitToDesktop()
{
    UKismetSystemLibrary::QuitGame(this, GetFirstLocalPlayerController(), EQuitPreference::Quit, false);
}

void UHorrorMenuSubsystem::HostEOSSession()
{
    UEOSLoginLibrary::CreateEOSSession(this);
}

void UHorrorMenuSubsystem::JoinEOSSession()
{
    UEOSLoginLibrary::FindAndJoinEOSSession(this);
}

bool UHorrorMenuSubsystem::IsOnlineUserLoggedIn() const
{
    if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
    {
        const IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
        return Identity.IsValid() && Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn;
    }

    return false;
}

FText UHorrorMenuSubsystem::GetOnlineStatusText() const
{
    if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
    {
        const IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
        if (Identity.IsValid() && Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn)
        {
            const FString Nickname = Identity->GetPlayerNickname(0);
            return Nickname.IsEmpty()
                ? FText::FromString(TEXT("EOS ONLINE"))
                : FText::FromString(FString::Printf(TEXT("EOS ONLINE  //  %s"), *Nickname.ToUpper()));
        }
    }

    return FText::FromString(TEXT("CONNECTING TO EPIC ONLINE SERVICES..."));
}

int32 UHorrorMenuSubsystem::GetQualityLevel() const
{
    const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
    return Settings ? FMath::Clamp(Settings->GetOverallScalabilityLevel(), 0, 3) : 2;
}

EWindowMode::Type UHorrorMenuSubsystem::GetWindowMode() const
{
    const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
    return Settings ? Settings->GetFullscreenMode() : EWindowMode::WindowedFullscreen;
}

FIntPoint UHorrorMenuSubsystem::GetScreenResolution() const
{
    const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
    return Settings ? Settings->GetScreenResolution() : FIntPoint(1920, 1080);
}

void UHorrorMenuSubsystem::ApplyVideoSettings(int32 QualityLevel, EWindowMode::Type WindowMode, FIntPoint Resolution)
{
    if (!GEngine)
    {
        return;
    }

    if (UGameUserSettings* Settings = GEngine->GetGameUserSettings())
    {
        Settings->SetOverallScalabilityLevel(FMath::Clamp(QualityLevel, 0, 3));
        Settings->SetFullscreenMode(WindowMode);
        Settings->SetScreenResolution(Resolution);
        Settings->ApplySettings(false);
        Settings->SaveSettings();
    }
}

void UHorrorMenuSubsystem::SetMasterVolume(float NewVolume)
{
    MasterVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
    ApplyRuntimePreferences();
    GConfig->SetFloat(MenuSettingsSection, TEXT("MasterVolume"), MasterVolume, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}

void UHorrorMenuSubsystem::SetLookSensitivity(float NewSensitivity)
{
    LookSensitivity = FMath::Clamp(NewSensitivity, 0.2f, 2.5f);
    GConfig->SetFloat(MenuSettingsSection, TEXT("LookSensitivity"), LookSensitivity, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}

void UHorrorMenuSubsystem::ApplyRuntimePreferences()
{
    if (UWorld* World = GetWorld())
    {
        if (FAudioDevice* AudioDevice = World->GetAudioDeviceRaw())
        {
            AudioDevice->SetTransientPrimaryVolume(MasterVolume);
        }
    }
}

bool UHorrorMenuSubsystem::IsPauseMenuOpen() const
{
    return ActiveMenu && ActiveMenu->IsInViewport() && ActiveMenu->GetMenuContext() == EHorrorMenuContext::Pause;
}

void UHorrorMenuSubsystem::HideActiveMenu()
{
    if (ActiveMenu)
    {
        ActiveMenu->RemoveFromParent();
        ActiveMenu = nullptr;
    }
}

APlayerController* UHorrorMenuSubsystem::GetFirstLocalPlayerController() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PlayerController = It->Get();
        if (PlayerController && PlayerController->IsLocalPlayerController())
        {
            return PlayerController;
        }
    }

    return UGameplayStatics::GetPlayerController(World, 0);
}

bool UHorrorMenuSubsystem::IsStartupMap(const UWorld* World) const
{
    return World && World->GetMapName().Contains(TEXT("GameStartupMap"));
}