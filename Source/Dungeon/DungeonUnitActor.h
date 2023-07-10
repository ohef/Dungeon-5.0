// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "lager/reader.hpp"
#include "Logic/DungeonGameState.h"
#include "Logic/unit.h"
#include "Utility/StoreConnectedClass.hpp"
#include "DungeonGameModeBase.h"
#include "PromiseFulfiller.h"
#include "Widget/DamageWidget.h"
#include "Widget/HealthBarWidget.h"
#include "Components/WidgetComponent.h"
#include "Templates/NonNullPointer.h"
#include "DungeonUnitActor.generated.h"

UCLASS()
class DUNGEON_API ADungeonUnitActor : public AActor, public FStoreConnectedClass<ADungeonUnitActor, TDungeonAction>
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ADungeonUnitActor();
	void CreateDynamicMaterials();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	FIntPoint lastPosition;
	using FReaderType = std::tuple<TOptional<FDungeonLogicUnit>, TOptional<FIntPoint>, TOptional<int>>;
	lager::reader<FReaderType> reader;

	void HookIntoStore(const FDungeonStore& store, TDungeonActionDispatched& eventToUse);
	void HandleGlobalEvent(const TDungeonAction& action);

	UFUNCTION(BlueprintImplementableEvent, Category="DungeonUnit")
	void React(FDungeonLogicUnit updatedState);

	UFUNCTION(BlueprintImplementableEvent, Category="DungeonUnit")
	void ReactCombatAction(FCombatAction updatedState);
	
	UPROPERTY(EditAnywhere)
	int Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* UnitIndicatorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* UnitIndicatorMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMeshComponent* UnitModel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UInterpToMovementComponent* InterpToMovementComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneComponent* PathRotation;
	UPROPERTY(Transient)
	TSubclassOf<UDamageWidget> DamageWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UWidgetComponent* HealthBarComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UWidgetComponent* DamageWidgetComponent;

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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequenceBase* CombatActionAnimationSubmit;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequenceBase* CombatActionAnimationWindUp;

	FTimerHandle timerHandle;

	virtual
	void Tick(float DeltaTime) override;
};
