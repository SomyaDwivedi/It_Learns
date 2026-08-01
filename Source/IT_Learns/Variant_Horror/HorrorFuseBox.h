#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Variant_Horror/HorrorInteractable.h"
#include "HorrorFuseBox.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** A one-shot, replicated fuse box that contributes to the emergency-power objective. */
UCLASS(BlueprintType)
class IT_LEARNS_API AHorrorFuseBox : public AActor, public IHorrorInteractable
{
    GENERATED_BODY()

public:
    AHorrorFuseBox();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual FText GetInteractionPrompt_Implementation(APawn* Interactor) const override;
    virtual bool CanInteract_Implementation(APawn* Interactor) const override;
    virtual bool Interact_Implementation(APawn* Interactor) override;

    UFUNCTION(BlueprintPure, Category="Horror|Puzzle")
    bool IsActivated() const { return bActivated; }

    UFUNCTION(BlueprintPure, Category="Horror|Puzzle")
    FName GetFuseId() const { return FuseId; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UStaticMeshComponent> Housing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UStaticMeshComponent> DoorPanel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UStaticMeshComponent> Lever;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UStaticMeshComponent> Indicator;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UPointLightComponent> IndicatorLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UTextRenderComponent> StatusLabel;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Horror|Puzzle")
    FName FuseId = NAME_None;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Horror|Puzzle")
    FText FuseLabel;

    UPROPERTY(ReplicatedUsing=OnRep_Activated, VisibleAnywhere, BlueprintReadOnly, Category="Horror|Puzzle")
    bool bActivated = false;

    UFUNCTION()
    void OnRep_Activated();

private:
    void ApplyVisualState();

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> IndicatorMaterial;
};
