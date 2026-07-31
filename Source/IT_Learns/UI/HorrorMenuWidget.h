#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericWindow.h"
#include "Blueprint/UserWidget.h"
#include "HorrorMenuWidget.generated.h"

struct FSlateDynamicImageBrush;
class SBox;
class STextBlock;
class UHorrorMenuSubsystem;

UENUM()
enum class EHorrorMenuContext : uint8
{
    Main,
    Pause
};

UCLASS()
class IT_LEARNS_API UHorrorMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UHorrorMenuWidget(const FObjectInitializer& ObjectInitializer);

    void Configure(UHorrorMenuSubsystem* InSubsystem, EHorrorMenuContext InContext);
    EHorrorMenuContext GetMenuContext() const { return MenuContext; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    TSharedRef<SWidget> BuildCurrentPage();
    TSharedRef<SWidget> BuildMainPage();
    TSharedRef<SWidget> BuildPausePage();
    TSharedRef<SWidget> BuildSettingsPage();
    TSharedRef<SWidget> BuildMenuButton(const FText& Label, const FOnClicked& OnClicked, bool bDanger = false) const;
    TSharedRef<SWidget> BuildSettingCycleRow(const FText& Label, const TAttribute<FText>& Value, const FOnClicked& OnClicked) const;
    TSharedRef<SWidget> BuildSliderRow(const FText& Label, float Value, const FOnFloatValueChanged& OnValueChanged, const TAttribute<FText>& ValueText) const;

    FReply HandleHostClicked();
    FReply HandleJoinClicked();
    FReply HandleSettingsClicked();
    FReply HandleResumeClicked();
    FReply HandleReturnToMenuClicked();
    FReply HandleQuitClicked();
    FReply HandleBackClicked();
    FReply HandleApplySettingsClicked();
    FReply HandleCycleQualityClicked();
    FReply HandleCycleWindowModeClicked();
    FReply HandleCycleResolutionClicked();

    void HandleMasterVolumeChanged(float NewValue);
    void HandleLookSensitivityChanged(float NewValue);
    void RefreshPage();
    void SetStatus(const FText& NewStatus, bool bOperationMessage);
    void LoadBackgroundBrush();

    FText GetQualityText() const;
    FText GetWindowModeText() const;
    FText GetResolutionText() const;
    FText GetMasterVolumeText() const;
    FText GetLookSensitivityText() const;

    TWeakObjectPtr<UHorrorMenuSubsystem> MenuSubsystem;
    EHorrorMenuContext MenuContext = EHorrorMenuContext::Main;
    bool bShowingSettings = false;
    bool bHasOperationStatus = false;

    int32 PendingQuality = 2;
    int32 PendingWindowModeIndex = 1;
    int32 PendingResolutionIndex = 2;
    TArray<FIntPoint> ResolutionOptions;

    TSharedPtr<SBox> PageContainer;
    TSharedPtr<STextBlock> StatusText;
    TSharedPtr<FSlateDynamicImageBrush> BackgroundBrush;
};