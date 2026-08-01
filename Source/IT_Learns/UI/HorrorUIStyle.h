#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"

/** Shared visual language for menus and in-game horror HUD elements. */
namespace HorrorUIStyle
{
    inline const FLinearColor Black(0.006f, 0.008f, 0.011f, 1.0f);
    inline const FLinearColor Panel(0.012f, 0.016f, 0.021f, 0.94f);
    inline const FLinearColor PanelSoft(0.025f, 0.030f, 0.036f, 0.88f);
    inline const FLinearColor Text(0.82f, 0.84f, 0.82f, 1.0f);
    inline const FLinearColor Muted(0.38f, 0.42f, 0.41f, 1.0f);
    inline const FLinearColor Accent(0.48f, 0.035f, 0.045f, 1.0f);
    inline const FLinearColor AccentHover(0.72f, 0.055f, 0.065f, 1.0f);
    inline const FLinearColor Success(0.30f, 0.58f, 0.40f, 1.0f);
    inline const FLinearColor Warning(0.78f, 0.55f, 0.16f, 1.0f);

    inline const FButtonStyle& GetButtonStyle(bool bDanger)
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
