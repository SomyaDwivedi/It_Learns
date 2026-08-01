#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HorrorObjectiveWidget.generated.h"

class AHorrorGameState;
class AGameStateBase;
class SBorder;
class STextBlock;
class SVerticalBox;
struct FHorrorObjectiveState;

/** Read-only mission HUD driven by the replicated horror game state. */
UCLASS()
class IT_LEARNS_API UHorrorObjectiveWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UHorrorObjectiveWidget(const FObjectInitializer& ObjectInitializer);

    /** Displays the local look-at interaction prompt without intercepting gameplay input. */
    void SetInteractionPrompt(const FText& Prompt, bool bCanInteract);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void HandleGameStateSet(AGameStateBase* NewGameState);
    void TryBindGameState();
    void RefreshObjectives();
    TSharedRef<SWidget> BuildObjectiveRow(const FHorrorObjectiveState& Objective) const;

    TWeakObjectPtr<AHorrorGameState> BoundGameState;
    TSharedPtr<SBorder> ObjectivePanel;
    TSharedPtr<STextBlock> ObjectiveCountText;
    TSharedPtr<SVerticalBox> ObjectiveList;
    TSharedPtr<SBorder> InteractionPanel;
    TSharedPtr<SBorder> InteractionKeyBorder;
    TSharedPtr<STextBlock> InteractionKeyText;
    TSharedPtr<STextBlock> InteractionPromptText;
};
