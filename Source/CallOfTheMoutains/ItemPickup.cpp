// CallOfTheMoutains - Item Pickup Implementation
// Overlap-based detection with E key to pick up

#include "ItemPickup.h"
#include "InventoryComponent.h"
#include "EquipmentComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/DataTable.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AItemPickup::AItemPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	// NOTE: Do NOT use ConstructorHelpers here - causes circular dependency crash
	// ItemDataTable will be loaded in BeginPlay or set in Blueprint

	// Create interaction sphere (root)
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetSphereRadius(InteractionRadius);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionSphere->SetGenerateOverlapEvents(true);
	RootComponent = InteractionSphere;

	// Create item mesh with physics enabled
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(RootComponent);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ItemMesh->SetCollisionResponseToAllChannels(ECR_Block);
	ItemMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	ItemMesh->SetSimulatePhysics(true);
	ItemMesh->SetEnableGravity(true);

	// Create point light for rarity glow
	RarityLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RarityLight"));
	RarityLight->SetupAttachment(ItemMesh);
	RarityLight->SetIntensity(LightIntensity);
	RarityLight->SetAttenuationRadius(LightRadius);
	RarityLight->SetCastShadows(false);
	RarityLight->SetLightColor(FLinearColor::White);
}

void AItemPickup::BeginPlay()
{
	Super::BeginPlay();

	// Load ItemDataTable at runtime if not set
	if (!ItemDataTable)
	{
		ItemDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/BluePrints/Data/ItemData"));
	}

	// Update sphere radius from property
	InteractionSphere->SetSphereRadius(InteractionRadius);

	// Bind overlap events
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AItemPickup::OnInteractionBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AItemPickup::OnInteractionEndOverlap);

	// Update light settings and set color based on item rarity
	if (RarityLight)
	{
		RarityLight->SetIntensity(LightIntensity);
		RarityLight->SetAttenuationRadius(LightRadius);
	}
	UpdateRarityLight();
}

void AItemPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsCollected)
	{
		CheckForPickupInput();
	}
}

FLinearColor AItemPickup::GetRarityColor(EItemRarity Rarity)
{
	switch (Rarity)
	{
	case EItemRarity::Common:
		// Dim white/gray for common items
		return FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);
	case EItemRarity::Uncommon:
		// Green glow
		return FLinearColor(0.2f, 0.8f, 0.2f, 1.0f);
	case EItemRarity::Rare:
		// Blue glow
		return FLinearColor(0.2f, 0.4f, 1.0f, 1.0f);
	case EItemRarity::Epic:
		// Purple glow
		return FLinearColor(0.6f, 0.2f, 0.9f, 1.0f);
	case EItemRarity::Legendary:
		// Golden/orange glow
		return FLinearColor(1.0f, 0.7f, 0.1f, 1.0f);
	default:
		return FLinearColor::White;
	}
}

void AItemPickup::UpdateRarityLight()
{
	if (!RarityLight)
	{
		return;
	}

	FItemData ItemData;
	if (GetItemData(ItemData))
	{
		FLinearColor RarityColor = GetRarityColor(ItemData.Rarity);
		RarityLight->SetLightColor(RarityColor);
	}
	else
	{
		// Default to common (dim white) if item data not found
		RarityLight->SetLightColor(GetRarityColor(EItemRarity::Common));
	}
}

void AItemPickup::CheckForPickupInput()
{
	if (!OverlappingPawn)
	{
		return;
	}

	// Get player controller for input
	APlayerController* PC = Cast<APlayerController>(OverlappingPawn->GetController());
	if (!PC)
	{
		return;
	}

	// Check E key press (not hold)
	bool bEDown = PC->IsInputKeyDown(EKeys::E);
	if (bEDown && !bEKeyWasDown)
	{
		TryPickup(OverlappingPawn);
	}
	bEKeyWasDown = bEDown;
}

void AItemPickup::OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	if (!Pawn->IsPlayerControlled())
	{
		return;
	}

	OverlappingPawn = Pawn;
	OnPickupFocused.Broadcast(this, true);
}

void AItemPickup::OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn && Pawn == OverlappingPawn)
	{
		OverlappingPawn = nullptr;
		bEKeyWasDown = false;
		OnPickupFocused.Broadcast(this, false);
	}
}

bool AItemPickup::TryPickup(APawn* Interactor)
{
	if (bIsCollected || !Interactor)
	{
		return false;
	}

	// Get the controller - inventory/equipment components live there, not on the pawn
	AController* Controller = Interactor->GetController();
	if (!Controller)
	{
		return false;
	}

	// Find inventory component on the CONTROLLER (not pawn)
	UInventoryComponent* Inventory = Controller->FindComponentByClass<UInventoryComponent>();
	if (!Inventory)
	{
		// Fallback: check pawn in case components are there
		Inventory = Interactor->FindComponentByClass<UInventoryComponent>();
	}

	if (!Inventory)
	{
		return false;
	}

	// CRITICAL: Share our DataTable with inventory if it doesn't have one
	if (!Inventory->ItemDataTable && ItemDataTable)
	{
		Inventory->ItemDataTable = ItemDataTable;
	}
	else if (!ItemDataTable && !Inventory->ItemDataTable)
	{
		return false;
	}

	// Also share with EquipmentComponent if present (check controller first, then pawn)
	UEquipmentComponent* Equipment = Controller->FindComponentByClass<UEquipmentComponent>();
	if (!Equipment)
	{
		Equipment = Interactor->FindComponentByClass<UEquipmentComponent>();
	}

	if (Equipment && !Equipment->ItemDataTable && ItemDataTable)
	{
		Equipment->ItemDataTable = ItemDataTable;
	}
	else if (Equipment && !Equipment->ItemDataTable && Inventory->ItemDataTable)
	{
		Equipment->ItemDataTable = Inventory->ItemDataTable;
	}

	// Try to add item to inventory (returns quantity added, 0 on failure)
	int32 Added = Inventory->AddItem(ItemID, Quantity);
	bool bSuccess = (Added > 0);

	if (bSuccess)
	{
		// Clear overlapping pawn reference before hiding
		OverlappingPawn = nullptr;
		OnPickupFocused.Broadcast(this, false);

		if (bRespawns)
		{
			HidePickup();
			GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AItemPickup::Respawn, RespawnDelay, false);
		}
		else
		{
			Destroy();
		}

		return true;
	}

	return false;
}

bool AItemPickup::GetItemData(FItemData& OutItemData) const
{
	if (!ItemDataTable || ItemID.IsNone())
	{
		return false;
	}

	FItemData* FoundData = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("GetItemData"));
	if (FoundData)
	{
		OutItemData = *FoundData;
		return true;
	}

	return false;
}

FText AItemPickup::GetPickupPrompt() const
{
	FItemData ItemData;
	FString ItemName;

	if (GetItemData(ItemData))
	{
		ItemName = ItemData.DisplayName.ToString();
	}
	else
	{
		ItemName = ItemID.ToString();
	}

	if (Quantity > 1)
	{
		return FText::FromString(FString::Printf(TEXT("Pick up %s x%d"), *ItemName, Quantity));
	}
	else
	{
		return FText::FromString(FString::Printf(TEXT("Pick up %s"), *ItemName));
	}
}

void AItemPickup::SetItem(FName NewItemID, int32 NewQuantity)
{
	ItemID = NewItemID;
	Quantity = FMath::Max(1, NewQuantity);

	// Update light color for new item
	UpdateRarityLight();
}

void AItemPickup::HidePickup()
{
	bIsCollected = true;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void AItemPickup::ShowPickup()
{
	bIsCollected = false;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
}

void AItemPickup::Respawn()
{
	ShowPickup();
}
