#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericWindow.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HorrorMenuSubsystem.generated.h"

class UHorrorMenuWidget;
enum class EHorrorMenuContext : uint8;

UCLASS()
class IT_LEARNS_API UHorrorMenuSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void ShowMainMenu();
    void TogglePauseMenu();
    void ResumeGame();
    void ReturnToMainMenu();
    void QuitToDesktop();

    void HostEOSSession();
    void JoinEOSSession();
    bool IsOnlineUserLoggedIn() const;
    FText GetOnlineStatusText() const;

    int32 GetQualityLevel() const;
    EWindowMode::Type GetWindowMode() const;
    FIntPoint GetScreenResolution() const;
    void ApplyVideoSettings(int32 QualityLevel, EWindowMode::Type WindowMode, FIntPoint Resolution);

    float GetMasterVolume() const { return MasterVolume; }
    void SetMasterVolume(float NewVolume);

    float GetLookSensitivity() const { return LookSensitivity; }
    void SetLookSensitivity(float NewSensitivity);

    bool IsPauseMenuOpen() const;

private:
    void HandlePostLoadMap(UWorld* LoadedWorld);
    void ShowMainMenuNextTick();
    void HideActiveMenu();
    void ApplyRuntimePreferences();
    void OpenStartupMap();
    void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
    APlayerController* GetFirstLocalPlayerController() const;
    bool IsStartupMap(const UWorld* World) const;

    UPROPERTY(Transient)
    TObjectPtr<UHorrorMenuWidget> ActiveMenu;

    FDelegateHandle PostLoadMapHandle;
    FDelegateHandle DestroySessionHandle;

    float MasterVolume = 0.8f;
    float LookSensitivity = 1.0f;
    bool bPendingMainMenu = false;
};