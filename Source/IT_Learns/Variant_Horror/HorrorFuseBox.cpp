#include "Variant_Horror/HorrorFuseBox.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Pawn.h"
#include "IT_Learns.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Variant_Horror/HorrorGameState.h"

namespace
{
    const FLinearColor OfflineColor(0.85f, 0.025f, 0.01f, 1.0f);
    const FLinearColor OnlineColor(0.02f, 0.8f, 0.16f, 1.0f);
}

AHorrorFuseBox::AHorrorFuseBox()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Housing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Housing"));
    Housing->SetupAttachment(SceneRoot);
    Housing->SetRelativeScale3D(FVector(0.14f, 0.38f, 0.52f));
    Housing->SetCollisionProfileName(TEXT("BlockAll"));

    DoorPanel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorPanel"));
    DoorPanel->SetupAttachment(SceneRoot);
    DoorPanel->SetRelativeLocation(FVector(8.0f, 0.0f, 0.0f));
    DoorPanel->SetRelativeScale3D(FVector(0.03f, 0.34f, 0.46f));
    DoorPanel->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Lever = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lever"));
    Lever->SetupAttachment(SceneRoot);
    Lever->SetRelativeLocation(FVector(13.0f, 0.0f, -4.0f));
    Lever->SetRelativeScale3D(FVector(0.03f, 0.04f, 0.17f));
    Lever->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Indicator = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Indicator"));
    Indicator->SetupAttachment(SceneRoot);
    Indicator->SetRelativeLocation(FVector(14.0f, 0.0f, 16.0f));
    Indicator->SetRelativeScale3D(FVector(0.055f));
    Indicator->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    IndicatorLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("IndicatorLight"));
    IndicatorLight->SetupAttachment(SceneRoot);
    IndicatorLight->SetRelativeLocation(FVector(20.0f, 0.0f, 16.0f));
    IndicatorLight->SetIntensity(520.0f);
    IndicatorLight->SetAttenuationRadius(115.0f);
    IndicatorLight->SetCastShadows(false);

    StatusLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusLabel"));
    StatusLabel->SetupAttachment(SceneRoot);
    StatusLabel->SetRelativeLocation(FVector(14.0f, 0.0f, -20.0f));
    StatusLabel->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    StatusLabel->SetHorizontalAlignment(EHTA_Center);
    StatusLabel->SetVerticalAlignment(EVRTA_TextCenter);
    StatusLabel->SetWorldSize(13.0f);
    StatusLabel->SetTextRenderColor(FColor::White);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Housing->SetStaticMesh(CubeMesh.Object);
        DoorPanel->SetStaticMesh(CubeMesh.Object);
        Lever->SetStaticMesh(CubeMesh.Object);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
    {
        Indicator->SetStaticMesh(SphereMesh.Object);
    }
}

void AHorrorFuseBox::BeginPlay()
{
    Super::BeginPlay();

    IndicatorMaterial = Indicator->CreateAndSetMaterialInstanceDynamic(0);
    ApplyVisualState();
}

void AHorrorFuseBox::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AHorrorFuseBox, bActivated);
}

FText AHorrorFuseBox::GetInteractionPrompt_Implementation(APawn* Interactor) const
{
    if (bActivated)
    {
        return NSLOCTEXT("HorrorFuseBox", "FuseOnlinePrompt", "Fuse box online");
    }

    const FText DisplayLabel = FuseLabel.IsEmpty()
        ? NSLOCTEXT("HorrorFuseBox", "EmergencyFuseLabel", "emergency fuse")
        : FuseLabel;
    return FText::Format(
        NSLOCTEXT("HorrorFuseBox", "ActivateFusePrompt", "Reset {0}"),
        DisplayLabel
    );
}

bool AHorrorFuseBox::CanInteract_Implementation(APawn* Interactor) const
{
    if (!Interactor || bActivated || FuseId.IsNone())
    {
        return false;
    }

    const AHorrorGameState* HorrorGameState = GetWorld() ? GetWorld()->GetGameState<AHorrorGameState>() : nullptr;
    return HorrorGameState && !HorrorGameState->IsFuseActivated(FuseId)
        && !HorrorGameState->IsEmergencyPowerRestored();
}

bool AHorrorFuseBox::Interact_Implementation(APawn* Interactor)
{
    if (!HasAuthority() || !CanInteract_Implementation(Interactor))
    {
        return false;
    }

    AHorrorGameState* HorrorGameState = GetWorld() ? GetWorld()->GetGameState<AHorrorGameState>() : nullptr;
    if (!HorrorGameState || !HorrorGameState->RegisterFuseActivation(FuseId))
    {
        return false;
    }

    bActivated = true;
    ApplyVisualState();
    ForceNetUpdate();

    UE_LOG(
        LogIT_Learns,
        Display,
        TEXT("Fuse box %s activated (%d/3)."),
        *FuseId.ToString(),
        HorrorGameState->GetActivatedFuseCount()
    );
    return true;
}

void AHorrorFuseBox::OnRep_Activated()
{
    ApplyVisualState();
}

void AHorrorFuseBox::ApplyVisualState()
{
    const FLinearColor StateColor = bActivated ? OnlineColor : OfflineColor;
    IndicatorLight->SetLightColor(StateColor);
    IndicatorLight->SetIntensity(bActivated ? 300.0f : 520.0f);
    Lever->SetRelativeRotation(FRotator(bActivated ? 30.0f : -30.0f, 0.0f, 0.0f));
    StatusLabel->SetText(
        bActivated
            ? NSLOCTEXT("HorrorFuseBox", "OnlineLabel", "ONLINE")
            : NSLOCTEXT("HorrorFuseBox", "OfflineLabel", "RESET FUSE")
    );
    StatusLabel->SetTextRenderColor(StateColor.ToFColorSRGB());

    if (IndicatorMaterial)
    {
        IndicatorMaterial->SetVectorParameterValue(TEXT("Color"), StateColor);
    }
}
