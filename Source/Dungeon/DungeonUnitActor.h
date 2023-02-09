// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "lager/reader.hpp"
#include "Logic/DungeonGameState.h"
#include "Logic/unit.h"
#include "Utility/StoreConnectedClass.hpp"
#include "DungeonGameModeBase.h"
#include "Widget/DamageWidget.h"
#include "Widget/HealthBarWidget.h"
#include "Components/WidgetComponent.h"
#include "DungeonUnitActor.generated.h"

UCLASS()
class DUNGEON_API ADungeonUnitActor : public AActor, public FStoreConnectedClass<ADungeonUnitActor, TDungeonAction>
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ADungeonUnitActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	int id;
	FIntPoint lastPosition;
	using FReaderType = std::tuple<FDungeonLogicUnit, FIntPoint, TOptional<int>>;
	lager::reader<FReaderType> reader;
	lager::reader<TTuple<FIntPoint, FIntPoint>> wew;

	void HookIntoStore();
	void HandleGlobalEvent(const TDungeonAction& action);

	UFUNCTION(BlueprintImplementableEvent, Category="DungeonUnit")
	void React(FDungeonLogicUnit updatedState);

	UFUNCTION(BlueprintImplementableEvent, Category="DungeonUnit")
	void ReactCombatAction(FCombatAction updatedState);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* UnitIndicatorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* UnitIndicatorMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UInterpToMovementComponent* InterpToMovementComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneComponent* PathRotation;
	UPROPERTY(Transient)
	TSubclassOf<UDamageWidget> DamageWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UWidgetComponent* HealthBarComponent;

	UPROPERTY(Transient)
	UDamageWidget* DamageWidget;
	UPROPERTY(Transient)
	TSubclassOf<UHealthBarWidget> HealthBarWidgetClass;
	UPROPERTY(Transient)
	UMaterial* HealthBarMaterial;
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* HealthBarMaterialDynamic;
	UPROPERTY(Transient)
	UHealthBarWidget* HealthBarWidget;
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* UnitIndicatorDynamic;
	UPROPERTY(Transient)
	UMaterialInstance* UnitIndicatorMaterial;

	virtual
	void Tick(float DeltaTime) override;
};
