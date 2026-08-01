#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HorrorInteractable.generated.h"

class APawn;

/** Implemented by world actors that can be used through the horror interaction trace. */
UINTERFACE(BlueprintType)
class IT_LEARNS_API UHorrorInteractable : public UInterface
{
    GENERATED_BODY()
};

class IT_LEARNS_API IHorrorInteractable
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Horror|Interaction")
    FText GetInteractionPrompt(APawn* Interactor) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Horror|Interaction")
    bool CanInteract(APawn* Interactor) const;

    /** Called only after the owning player's server RPC validates range and line of sight. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Horror|Interaction")
    bool Interact(APawn* Interactor);
};
