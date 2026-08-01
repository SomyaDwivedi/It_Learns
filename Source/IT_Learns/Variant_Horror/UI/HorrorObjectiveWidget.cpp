#include "Variant_Horror/UI/HorrorObjectiveWidget.h"

#include "Engine/World.h"
#include "Styling/CoreStyle.h"
#include "UI/HorrorUIStyle.h"
#include "Variant_Horror/HorrorGameState.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

UHorrorObjectiveWidget::UHorrorObjectiveWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(false);
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

TSharedRef<SWidget> UHorrorObjectiveWidget::RebuildWidget()
{
    TSharedRef<SOverlay> Root = SNew(SOverlay)
        .Visibility(EVisibility::HitTestInvisible);

    Root->AddSlot()
    .HAlign(HAlign_Right)
    .VAlign(VAlign_Top)
    .Padding(FMargin(32.0f, 38.0f, 38.0f, 32.0f))
    [
        SNew(SBox)
        .WidthOverride(430.0f)
        [
            SAssignNew(ObjectivePanel, SBorder)
            .Visibility(EVisibility::Collapsed)
            .BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
            .BorderBackgroundColor(HorrorUIStyle::Panel)
            .Padding(FMargin(22.0f, 18.0f, 22.0f, 20.0f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
                    [
                        SNew(SBorder)
                        .BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
                        .BorderBackgroundColor(HorrorUIStyle::Accent)
                        .Padding(FMargin(2.0f, 9.0f))
                    ]
                    + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(NSLOCTEXT("HorrorObjectives", "ObjectivesHeader", "CURRENT OBJECTIVES"))
                        .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 14))
                        .ColorAndOpacity(HorrorUIStyle::Text)
                    ]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                    [
                        SAssignNew(ObjectiveCountText, STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 9))
                        .ColorAndOpacity(HorrorUIStyle::Muted)
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
                [
                    SAssignNew(ObjectiveList, SVerticalBox)
                ]
            ]
        ]
    ];

    Root->AddSlot()
    .HAlign(HAlign_Center)
    .VAlign(VAlign_Bottom)
    .Padding(FMargin(30.0f, 30.0f, 30.0f, 92.0f))
    [
        SAssignNew(InteractionPanel, SBorder)
        .Visibility(EVisibility::Collapsed)
        .BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
        .BorderBackgroundColor(HorrorUIStyle::Panel)
        .Padding(FMargin(12.0f, 9.0f, 16.0f, 9.0f))
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [
                SAssignNew(InteractionKeyBorder, SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
                .BorderBackgroundColor(HorrorUIStyle::Accent)
                .Padding(FMargin(10.0f, 5.0f))
                [
                    SAssignNew(InteractionKeyText, STextBlock)
                    .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 11))
                    .ColorAndOpacity(HorrorUIStyle::Text)
                ]
            ]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.0f, 0.0f, 0.0f, 0.0f)
            [
                SAssignNew(InteractionPromptText, STextBlock)
                .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 11))
                .ColorAndOpacity(HorrorUIStyle::Text)
            ]
        ]
    ];

    return Root;
}

void UHorrorObjectiveWidget::SetInteractionPrompt(const FText& Prompt, bool bCanInteract)
{
    if (!InteractionPanel.IsValid() || !InteractionKeyBorder.IsValid()
        || !InteractionKeyText.IsValid() || !InteractionPromptText.IsValid())
    {
        return;
    }

    if (Prompt.IsEmpty())
    {
        InteractionPanel->SetVisibility(EVisibility::Collapsed);
        return;
    }

    InteractionPanel->SetVisibility(EVisibility::HitTestInvisible);
    InteractionKeyBorder->SetBorderBackgroundColor(
        bCanInteract ? HorrorUIStyle::Accent : HorrorUIStyle::Success
    );
    InteractionKeyText->SetText(
        bCanInteract
            ? NSLOCTEXT("HorrorInteraction", "InteractKey", "E")
            : NSLOCTEXT("HorrorInteraction", "OnlineState", "ONLINE")
    );
    InteractionPromptText->SetText(Prompt);
    InteractionPromptText->SetColorAndOpacity(
        bCanInteract ? HorrorUIStyle::Text : HorrorUIStyle::Muted
    );
}

void UHorrorObjectiveWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (UWorld* World = GetWorld())
    {
        World->GameStateSetEvent.RemoveAll(this);
        World->GameStateSetEvent.AddUObject(this, &UHorrorObjectiveWidget::HandleGameStateSet);
    }

    TryBindGameState();
}

void UHorrorObjectiveWidget::NativeDestruct()
{
    if (BoundGameState.IsValid())
    {
        BoundGameState->OnObjectivesChanged().RemoveAll(this);
    }

    BoundGameState.Reset();
    if (UWorld* World = GetWorld())
    {
        World->GameStateSetEvent.RemoveAll(this);
    }

    Super::NativeDestruct();
}

void UHorrorObjectiveWidget::HandleGameStateSet(AGameStateBase* NewGameState)
{
    (void)NewGameState;
    TryBindGameState();
}

void UHorrorObjectiveWidget::TryBindGameState()
{
    AHorrorGameState* CurrentGameState = GetWorld() ? GetWorld()->GetGameState<AHorrorGameState>() : nullptr;
    if (CurrentGameState == BoundGameState.Get())
    {
        RefreshObjectives();
        return;
    }

    if (BoundGameState.IsValid())
    {
        BoundGameState->OnObjectivesChanged().RemoveAll(this);
    }

    BoundGameState = CurrentGameState;
    if (BoundGameState.IsValid())
    {
        BoundGameState->OnObjectivesChanged().AddUObject(this, &UHorrorObjectiveWidget::RefreshObjectives);
    }

    RefreshObjectives();
}

void UHorrorObjectiveWidget::RefreshObjectives()
{
    if (!ObjectivePanel.IsValid() || !ObjectiveList.IsValid())
    {
        return;
    }

    ObjectiveList->ClearChildren();

    int32 VisibleObjectiveCount = 0;
    int32 ActiveObjectiveCount = 0;
    if (BoundGameState.IsValid())
    {
        for (const FHorrorObjectiveState& Objective : BoundGameState->GetObjectives())
        {
            if (!Objective.IsVisibleInHUD())
            {
                continue;
            }

            if (Objective.Status == EHorrorObjectiveStatus::Active)
            {
                ++ActiveObjectiveCount;
            }

            ObjectiveList->AddSlot()
            .AutoHeight()
            .Padding(0.0f, VisibleObjectiveCount == 0 ? 0.0f : 7.0f, 0.0f, 0.0f)
            [
                BuildObjectiveRow(Objective)
            ];
            ++VisibleObjectiveCount;
        }
    }

    ObjectivePanel->SetVisibility(VisibleObjectiveCount > 0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
    if (ObjectiveCountText.IsValid())
    {
        ObjectiveCountText->SetText(FText::Format(
            NSLOCTEXT("HorrorObjectives", "ActiveObjectiveCount", "{0} ACTIVE"),
            FText::AsNumber(ActiveObjectiveCount)
        ));
    }
}

TSharedRef<SWidget> UHorrorObjectiveWidget::BuildObjectiveRow(const FHorrorObjectiveState& Objective) const
{
    const bool bCompleted = Objective.Status == EHorrorObjectiveStatus::Completed;
    const bool bFailed = Objective.Status == EHorrorObjectiveStatus::Failed;
    const FLinearColor StateColor = bCompleted
        ? HorrorUIStyle::Success
        : (bFailed ? HorrorUIStyle::Warning : HorrorUIStyle::AccentHover);

    FText StateText;
    if (bCompleted)
    {
        StateText = NSLOCTEXT("HorrorObjectives", "ObjectiveCompleted", "COMPLETED");
    }
    else if (bFailed)
    {
        StateText = NSLOCTEXT("HorrorObjectives", "ObjectiveFailed", "FAILED");
    }
    else if (Objective.bOptional)
    {
        StateText = NSLOCTEXT("HorrorObjectives", "ObjectiveOptional", "OPTIONAL");
    }

    return SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
        .BorderBackgroundColor(HorrorUIStyle::PanelSoft)
        .Padding(FMargin(15.0f, 12.0f, 15.0f, 13.0f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(Objective.Title)
                    .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 13))
                    .ColorAndOpacity(bCompleted || bFailed ? HorrorUIStyle::Muted : HorrorUIStyle::Text)
                ]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Visibility(StateText.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible)
                    .Text(StateText)
                    .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 8))
                    .ColorAndOpacity(StateColor)
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text(Objective.Description)
                .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10))
                .ColorAndOpacity(HorrorUIStyle::Muted)
                .AutoWrapText(true)
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 0.0f)
            [
                SNew(SHorizontalBox)
                .Visibility(Objective.TargetProgress > 0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
                + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
                [
                    SNew(SProgressBar)
                    .Style(&FCoreStyle::Get().GetWidgetStyle<FProgressBarStyle>(TEXT("ProgressBar")))
                    .Percent(Objective.GetProgressFraction())
                    .FillColorAndOpacity(StateColor)
                ]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Text(Objective.GetProgressText())
                    .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 10))
                    .ColorAndOpacity(bCompleted ? HorrorUIStyle::Success : HorrorUIStyle::Text)
                ]
            ]
        ];
}
