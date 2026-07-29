#include "UI/HorrorMenuWidget.h"

#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "UI/HorrorMenuSubsystem.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateDynamicImageBrush.h"

namespace HorrorMenuStyle
{
    const FLinearColor Black(0.006f, 0.008f, 0.011f, 1.0f);
    const FLinearColor Panel(0.012f, 0.016f, 0.021f, 0.94f);
    const FLinearColor PanelSoft(0.025f, 0.030f, 0.036f, 0.88f);
    const FLinearColor Text(0.82f, 0.84f, 0.82f, 1.0f);
    const FLinearColor Muted(0.38f, 0.42f, 0.41f, 1.0f);
    const FLinearColor Accent(0.48f, 0.035f, 0.045f, 1.0f);
    const FLinearColor AccentHover(0.72f, 0.055f, 0.065f, 1.0f);

    const FButtonStyle& GetButtonStyle(bool bDanger)
    {
        static const FButtonStyle StandardStyle = []
        {
            FButtonStyle Style;
            Style.SetNormal(FSlateColorBrush(FLinearColor(0.025f, 0.030f, 0.035f, 0.94f)));
            Style.SetHovered(FSlateColorBrush(FLinearColor(0.075f, 0.025f, 0.030f, 0.98f)));
            Style.SetPressed(FSlateColorBrush(FLinearColor(0.15f, 0.025f, 0.030f, 1.0f)));
            Style.SetDisabled(FSlateColorBrush(FLinearColor(0.018f, 0.020f, 0.023f, 0.75f)));
            Style.SetNormalPadding(FMargin(18.0f, 12.0f));
            Style.SetPressedPadding(FMargin(19.0f, 13.0f, 17.0f, 11.0f));
            return Style;
        }();

        static const FButtonStyle DangerStyle = []
        {
            FButtonStyle Style;
            Style.SetNormal(FSlateColorBrush(FLinearColor(0.08f, 0.012f, 0.016f, 0.96f)));
            Style.SetHovered(FSlateColorBrush(FLinearColor(0.28f, 0.018f, 0.025f, 1.0f)));
            Style.SetPressed(FSlateColorBrush(FLinearColor(0.48f, 0.025f, 0.035f, 1.0f)));
            Style.SetNormalPadding(FMargin(18.0f, 12.0f));
            Style.SetPressedPadding(FMargin(19.0f, 13.0f, 17.0f, 11.0f));
            return Style;
        }();

        return bDanger ? DangerStyle : StandardStyle;
    }
}

UHorrorMenuWidget::UHorrorMenuWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(true);
    ResolutionOptions = {
        FIntPoint(1280, 720),
        FIntPoint(1600, 900),
        FIntPoint(1920, 1080),
        FIntPoint(2560, 1440),
        FIntPoint(3840, 2160)
    };
}

void UHorrorMenuWidget::Configure(UHorrorMenuSubsystem* InSubsystem, EHorrorMenuContext InContext)
{
    MenuSubsystem = InSubsystem;
    MenuContext = InContext;
    bShowingSettings = false;
    bHasOperationStatus = false;

    if (MenuSubsystem.IsValid())
    {
        PendingQuality = MenuSubsystem->GetQualityLevel();

        switch (MenuSubsystem->GetWindowMode())
        {
            case EWindowMode::Windowed: PendingWindowModeIndex = 0; break;
            case EWindowMode::Fullscreen: PendingWindowModeIndex = 2; break;
            default: PendingWindowModeIndex = 1; break;
        }

        const FIntPoint CurrentResolution = MenuSubsystem->GetScreenResolution();
        PendingResolutionIndex = ResolutionOptions.IndexOfByKey(CurrentResolution);
        if (PendingResolutionIndex == INDEX_NONE)
        {
            ResolutionOptions.Add(CurrentResolution);
            PendingResolutionIndex = ResolutionOptions.Num() - 1;
        }
    }
}

TSharedRef<SWidget> UHorrorMenuWidget::RebuildWidget()
{
    LoadBackgroundBrush();

    TSharedRef<SOverlay> Root = SNew(SOverlay);

    Root->AddSlot()
    [
        SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
        .BorderBackgroundColor(HorrorMenuStyle::Black)
    ];

    if (BackgroundBrush.IsValid())
    {
        Root->AddSlot()
        [
            SNew(SScaleBox)
            .Stretch(EStretch::ScaleToFill)
            [
                SNew(SImage)
                .Image(BackgroundBrush.Get())
                .ColorAndOpacity(FLinearColor(0.62f, 0.62f, 0.62f, 1.0f))
            ]
        ];
    }

    Root->AddSlot()
    [
        SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
        .BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, MenuContext == EHorrorMenuContext::Main ? 0.42f : 0.72f))
    ];

    Root->AddSlot()
    .HAlign(MenuContext == EHorrorMenuContext::Main ? HAlign_Left : HAlign_Center)
    .VAlign(VAlign_Center)
    .Padding(MenuContext == EHorrorMenuContext::Main ? FMargin(110.0f, 60.0f) : FMargin(40.0f))
    [
        SAssignNew(PageContainer, SBox)
        .WidthOverride(bShowingSettings ? 720.0f : (MenuContext == EHorrorMenuContext::Main ? 470.0f : 520.0f))
        [
            BuildCurrentPage()
        ]
    ];

    return Root;
}

TSharedRef<SWidget> UHorrorMenuWidget::BuildCurrentPage()
{
    if (bShowingSettings)
    {
        return BuildSettingsPage();
    }

    return MenuContext == EHorrorMenuContext::Main ? BuildMainPage() : BuildPausePage();
}

TSharedRef<SWidget> UHorrorMenuWidget::BuildMainPage()
{
    return SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
        .BorderBackgroundColor(HorrorMenuStyle::Panel)
        .Padding(FMargin(42.0f, 36.0f, 42.0f, 30.0f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("IT LEARNS")))
                .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 52))
                .ColorAndOpacity(HorrorMenuStyle::Text)
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(2.0f, 2.0f, 0.0f, 18.0f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("A CO-OP SURVIVAL HORROR EXPERIENCE")))
                .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 11))
                .ColorAndOpacity(HorrorMenuStyle::Muted)
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 24.0f)
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
                .BorderBackgroundColor(HorrorMenuStyle::Accent)
                .Padding(FMargin(0.0f, 1.0f))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
            [
                BuildMenuButton(FText::FromString(TEXT("HOST GAME")), FOnClicked::CreateUObject(this, &UHorrorMenuWidget::HandleHostClicked))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
            [
                BuildMenuButton(FText::FromString(TEXT("JOIN GAME")), FOnClicked::CreateUObject(this, &UHorrorMenuWidget::HandleJoinClicked))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
            [
                BuildMenuButton(FText::FromString(TEXT("SETTINGS")), FOnClicked::CreateUObject(this, &UHorrorMenuWidget::HandleSettingsClicked))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
            [
                BuildMenuButton(FText::FromString(TEXT("QUIT")), FOnClicked::CreateUObject(this, &UHorrorMenuWidget::HandleQuitClicked), true)
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 25.0f, 0.0f, 0.0f)
            [
                SAssignNew(StatusText, STextBlock)
                .Text(MenuSubsystem.IsValid() ? MenuSubsystem->GetOnlineStatusText() : FText::GetEmpty())
                .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 10))
                .ColorAndOpacity(HorrorMenuStyle::Muted)
            ]
        ];
}

TSharedRef<SWidget> UHorrorMenuWidget::BuildPausePage()
{
    return SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
        .BorderBackgroundColor(HorrorMenuStyle::Panel)
        .Padding(FMargin(44.0f, 36.0f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("PAUSED")))
                .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 42))
                .ColorAndOpacity(HorrorMenuStyle::Text)
            ]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 4.0f, 0.0f, 22.0f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("ONLINE SESSIONS CONTINUE WHILE THIS MENU IS OPEN")))
                .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 9))
                .ColorAndOpacity(HorrorMenuStyle::Muted)
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
            [
                BuildMenuButton(FText::FromString(TEXT("RESUME")), FOnClicked::CreateUObject(this, &UHorrorMenuWidget::HandleResumeClicked))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
            [
                BuildMenuButton(FText::FromString(TEXT("SETTINGS")), FOnClicked::CreateUObject(this, &UHorrorMenuWidget::HandleSettingsClicked))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
            [
                BuildMenuButton(FText::FromString(TEXT("RETURN TO MAIN MENU")), FOnClicked::CreateUObject(this, &UHorrorMenuWidget::HandleReturnToMenuClicked), true)
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
            [
                BuildMenuButton(FText::FromString(TEXT("QUIT TO DESKTOP")), FOnClicked::CreateUObject(this, &UHorrorMenuWidget::HandleQuitClicked), true)
            ]
        ];
}

TSharedRef<SWidget> UHorrorMenuWidget::BuildSettingsPage()
{
    return SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
        .BorderBackgroundColor(HorrorMenuStyle::Panel)
        .Padding(FMargin(46.0f, 36.0f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("SETTINGS")))
                .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 38))
                .ColorAndOpacity(HorrorMenuStyle::Text)
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 22.0f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("VIDEO  /  AUDIO  /  CONTROLS")))
                .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10))
                .ColorAndOpacity(HorrorMenuStyle::Muted)
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
            [
                BuildSettingCycleRow(FText::FromString(TEXT("GRAPHICS QUALITY")), TAttribute<FText>::CreateLambda([this] { return GetQualityText(); }), FOnClicked::CreateUObject(this, &UHorrorMenuWidget::HandleCycleQualityClicked))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
            [
                BuildSettingCycleRow(FText::FromString(TEXT("DISPLAY MODE")), TAttribute<FText>::CreateLambda([this] { return GetWindowModeText(); }), FOnClicked::CreateUObject(this, &UHorrorMenuWidget::HandleCycleWindowModeClicked))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
            [
                BuildSettingCycleRow(FText::FromString(TEXT("RESOLUTION")), TAttribute<FText>::CreateLambda([this] { return GetResolutionText(); }), FOnClicked::CreateUObject(this, &UHorrorMenuWidget::HandleCycleResolutionClicked))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 4.0f)
            [
                BuildSliderRow(
                    FText::FromString(TEXT("MASTER VOLUME")),
                    MenuSubsystem.IsValid() ? MenuSubsystem->GetMasterVolume() : 0.8f,
                    FOnFloatValueChanged::CreateUObject(this, &UHorrorMenuWidget::HandleMasterVolumeChanged),
                    TAttribute<FText>::CreateLambda([this] { return GetMasterVolumeText(); })
                )
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
            [
                BuildSliderRow(
                    FText::FromString(TEXT("LOOK SENSITIVITY")),
                    MenuSubsystem.IsValid() ? (MenuSubsystem->GetLookSensitivity() - 0.2f) / 2.3f : 0.35f,
                    FOnFloatValueChanged::CreateUObject(this, &UHorrorMenuWidget::HandleLookSensitivityChanged),
                    TAttribute<FText>::CreateLambda([this] { return GetLookSensitivityText(); })
                )
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 24.0f, 0.0f, 5.0f)
            [
                BuildMenuButton(FText::FromString(TEXT("APPLY VIDEO SETTINGS")), FOnClicked::CreateUObject(this, &UHorrorMenuWidget::HandleApplySettingsClicked))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
            [
                BuildMenuButton(FText::FromString(TEXT("BACK")), FOnClicked::CreateUObject(this, &UHorrorMenuWidget::HandleBackClicked))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 18.0f, 0.0f, 0.0f)
            [
                SAssignNew(StatusText, STextBlock)
                .Text(FText::FromString(TEXT("CHANGES TO AUDIO AND CONTROLS APPLY IMMEDIATELY")))
                .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 9))
                .ColorAndOpacity(HorrorMenuStyle::Muted)
            ]
        ];
}

TSharedRef<SWidget> UHorrorMenuWidget::BuildMenuButton(const FText& Label, const FOnClicked& OnClicked, bool bDanger) const
{
    return SNew(SBox)
        .HeightOverride(56.0f)
        [
            SNew(SButton)
            .ButtonStyle(&HorrorMenuStyle::GetButtonStyle(bDanger))
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Center)
            .OnClicked(OnClicked)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 1.0f, 14.0f, 1.0f)
                [
                    SNew(SBorder)
                    .BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
                    .BorderBackgroundColor(bDanger ? HorrorMenuStyle::AccentHover : HorrorMenuStyle::Accent)
                    .Padding(FMargin(2.0f, 0.0f))
                ]
                + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(Label)
                    .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 17))
                    .ColorAndOpacity(HorrorMenuStyle::Text)
                ]
            ]
        ];
}

TSharedRef<SWidget> UHorrorMenuWidget::BuildSettingCycleRow(const FText& Label, const TAttribute<FText>& Value, const FOnClicked& OnClicked) const
{
    return SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
        .BorderBackgroundColor(HorrorMenuStyle::PanelSoft)
        .Padding(FMargin(16.0f, 10.0f))
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(Label)
                .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 12))
                .ColorAndOpacity(HorrorMenuStyle::Muted)
            ]
            + SHorizontalBox::Slot().AutoWidth()
            [
                SNew(SButton)
                .ButtonStyle(&HorrorMenuStyle::GetButtonStyle(false))
                .ContentPadding(FMargin(18.0f, 7.0f))
                .OnClicked(OnClicked)
                [
                    SNew(STextBlock)
                    .Text(Value)
                    .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 12))
                    .ColorAndOpacity(HorrorMenuStyle::Text)
                    .MinDesiredWidth(150.0f)
                    .Justification(ETextJustify::Center)
                ]
            ]
        ];
}

TSharedRef<SWidget> UHorrorMenuWidget::BuildSliderRow(const FText& Label, float Value, const FOnFloatValueChanged& OnValueChanged, const TAttribute<FText>& ValueText) const
{
    return SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
        .BorderBackgroundColor(HorrorMenuStyle::PanelSoft)
        .Padding(FMargin(16.0f, 12.0f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [
                    SNew(STextBlock)
                    .Text(Label)
                    .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 12))
                    .ColorAndOpacity(HorrorMenuStyle::Muted)
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(STextBlock)
                    .Text(ValueText)
                    .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 12))
                    .ColorAndOpacity(HorrorMenuStyle::Text)
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
            [
                SNew(SSlider)
                .Value(Value)
                .StepSize(0.01f)
                .SliderBarColor(HorrorMenuStyle::Muted)
                .SliderHandleColor(HorrorMenuStyle::AccentHover)
                .OnValueChanged(OnValueChanged)
            ]
        ];
}

FReply UHorrorMenuWidget::HandleHostClicked()
{
    if (!MenuSubsystem.IsValid() || !MenuSubsystem->IsOnlineUserLoggedIn())
    {
        SetStatus(FText::FromString(TEXT("EPIC LOGIN IS STILL IN PROGRESS  -  PLEASE WAIT")), false);
        return FReply::Handled();
    }

    SetStatus(FText::FromString(TEXT("CREATING EOS SESSION...")), true);
    MenuSubsystem->HostEOSSession();
    return FReply::Handled();
}

FReply UHorrorMenuWidget::HandleJoinClicked()
{
    if (!MenuSubsystem.IsValid() || !MenuSubsystem->IsOnlineUserLoggedIn())
    {
        SetStatus(FText::FromString(TEXT("EPIC LOGIN IS STILL IN PROGRESS  -  PLEASE WAIT")), false);
        return FReply::Handled();
    }

    SetStatus(FText::FromString(TEXT("SEARCHING FOR SURVIVORS...")), true);
    MenuSubsystem->JoinEOSSession();
    return FReply::Handled();
}

FReply UHorrorMenuWidget::HandleSettingsClicked()
{
    bShowingSettings = true;
    bHasOperationStatus = false;
    RefreshPage();
    return FReply::Handled();
}

FReply UHorrorMenuWidget::HandleResumeClicked()
{
    if (MenuSubsystem.IsValid())
    {
        MenuSubsystem->ResumeGame();
    }
    return FReply::Handled();
}

FReply UHorrorMenuWidget::HandleReturnToMenuClicked()
{
    if (MenuSubsystem.IsValid())
    {
        MenuSubsystem->ReturnToMainMenu();
    }
    return FReply::Handled();
}

FReply UHorrorMenuWidget::HandleQuitClicked()
{
    if (MenuSubsystem.IsValid())
    {
        MenuSubsystem->QuitToDesktop();
    }
    return FReply::Handled();
}

FReply UHorrorMenuWidget::HandleBackClicked()
{
    bShowingSettings = false;
    bHasOperationStatus = false;
    RefreshPage();
    return FReply::Handled();
}

FReply UHorrorMenuWidget::HandleApplySettingsClicked()
{
    if (MenuSubsystem.IsValid() && ResolutionOptions.IsValidIndex(PendingResolutionIndex))
    {
        const EWindowMode::Type Mode = PendingWindowModeIndex == 0
            ? EWindowMode::Windowed
            : (PendingWindowModeIndex == 2 ? EWindowMode::Fullscreen : EWindowMode::WindowedFullscreen);

        MenuSubsystem->ApplyVideoSettings(PendingQuality, Mode, ResolutionOptions[PendingResolutionIndex]);
        SetStatus(FText::FromString(TEXT("VIDEO SETTINGS APPLIED")), true);
    }

    return FReply::Handled();
}

FReply UHorrorMenuWidget::HandleCycleQualityClicked()
{
    PendingQuality = (PendingQuality + 1) % 4;
    return FReply::Handled();
}

FReply UHorrorMenuWidget::HandleCycleWindowModeClicked()
{
    PendingWindowModeIndex = (PendingWindowModeIndex + 1) % 3;
    return FReply::Handled();
}

FReply UHorrorMenuWidget::HandleCycleResolutionClicked()
{
    if (!ResolutionOptions.IsEmpty())
    {
        PendingResolutionIndex = (PendingResolutionIndex + 1) % ResolutionOptions.Num();
    }
    return FReply::Handled();
}

void UHorrorMenuWidget::HandleMasterVolumeChanged(float NewValue)
{
    if (MenuSubsystem.IsValid())
    {
        MenuSubsystem->SetMasterVolume(NewValue);
    }
}

void UHorrorMenuWidget::HandleLookSensitivityChanged(float NewValue)
{
    if (MenuSubsystem.IsValid())
    {
        MenuSubsystem->SetLookSensitivity(0.2f + NewValue * 2.3f);
    }
}

void UHorrorMenuWidget::RefreshPage()
{
    StatusText.Reset();
    if (PageContainer.IsValid())
    {
        PageContainer->SetWidthOverride(bShowingSettings ? 720.0f : (MenuContext == EHorrorMenuContext::Main ? 470.0f : 520.0f));
        PageContainer->SetContent(BuildCurrentPage());
    }
}

void UHorrorMenuWidget::SetStatus(const FText& NewStatus, bool bOperationMessage)
{
    bHasOperationStatus = bOperationMessage;
    if (StatusText.IsValid())
    {
        StatusText->SetText(NewStatus);
    }
}

void UHorrorMenuWidget::LoadBackgroundBrush()
{
    if (BackgroundBrush.IsValid() || !FSlateApplication::IsInitialized())
    {
        return;
    }

    const FString BackgroundPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("UI/Horror/Art/HorrorMenuBackground.png"));
    if (!FPaths::FileExists(BackgroundPath))
    {
        return;
    }
    const FName ResourceName(*BackgroundPath);
    const FIntPoint ImageSize = FSlateApplication::Get().GetRenderer()->GenerateDynamicImageResource(ResourceName);
    if (ImageSize.X > 0 && ImageSize.Y > 0)
    {
        BackgroundBrush = MakeShared<FSlateDynamicImageBrush>(
            ResourceName,
            FVector2D(static_cast<float>(ImageSize.X), static_cast<float>(ImageSize.Y))
        );
    }
}

FText UHorrorMenuWidget::GetQualityText() const
{
    static const TCHAR* Names[] = { TEXT("LOW"), TEXT("MEDIUM"), TEXT("HIGH"), TEXT("EPIC") };
    return FText::FromString(Names[FMath::Clamp(PendingQuality, 0, 3)]);
}

FText UHorrorMenuWidget::GetWindowModeText() const
{
    static const TCHAR* Names[] = { TEXT("WINDOWED"), TEXT("BORDERLESS"), TEXT("FULLSCREEN") };
    return FText::FromString(Names[FMath::Clamp(PendingWindowModeIndex, 0, 2)]);
}

FText UHorrorMenuWidget::GetResolutionText() const
{
    if (!ResolutionOptions.IsValidIndex(PendingResolutionIndex))
    {
        return FText::FromString(TEXT("1920 x 1080"));
    }

    const FIntPoint Resolution = ResolutionOptions[PendingResolutionIndex];
    return FText::FromString(FString::Printf(TEXT("%d x %d"), Resolution.X, Resolution.Y));
}

FText UHorrorMenuWidget::GetMasterVolumeText() const
{
    const float Value = MenuSubsystem.IsValid() ? MenuSubsystem->GetMasterVolume() : 0.8f;
    return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.0f)));
}

FText UHorrorMenuWidget::GetLookSensitivityText() const
{
    const float Value = MenuSubsystem.IsValid() ? MenuSubsystem->GetLookSensitivity() : 1.0f;
    return FText::FromString(FString::Printf(TEXT("%.2f"), Value));
}

void UHorrorMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bShowingSettings && !bHasOperationStatus && StatusText.IsValid() && MenuSubsystem.IsValid())
    {
        StatusText->SetText(MenuSubsystem->GetOnlineStatusText());
    }
}

FReply UHorrorMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape && MenuContext == EHorrorMenuContext::Pause)
    {
        if (bShowingSettings)
        {
            return HandleBackClicked();
        }

        return HandleResumeClicked();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}