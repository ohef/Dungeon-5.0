// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <Engine/Classes/Components/TimelineComponent.h>
#include <Logic/DungeonGameState.h>

#include "CoreMinimal.h"
#include "SingleSubmitHandler.h"
#include "TargetsAvailableId.h"
#include "Actions/CombatAction.h"
#include "Actor/MapCursorPawn.h"
#include "Blueprint/UserWidget.h"
#include "Curves/CurveVector.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/GameModeBase.h"
#include "lager/store.hpp"
#include "lager/event_loop/queue.hpp"

#include "DungeonGameModeBase.generated.h"

class ADungeonGameModeBase;
class UDungeonMainWidget;
class UTileVisualizationComponent;
struct FSelectingGameState;
struct ProgramState;

USTRUCT(BlueprintType)
struct FAbilityParams : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Dungeon)
	UMaterialInstance* coloring;
};

USTRUCT()
struct FStructThing : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int ID;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int HP;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int Movement;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCombatActionEvent, FCombatAction, action);

struct FCoerceToFText
{
	template <typename T>
	static typename TEnableIf<TIsArithmetic<T>::Value, FText>::Type Value(T thing)
	{
		return FText::AsNumber(thing);
	}

	static FText Value(FString thing)
	{
		return FText::FromString(thing);
	}
};

#define CREATE_GETTER_FOR_PROPERTY(managedPointer, fieldName) \
FText Get##managedPointer####fieldName() const \
{ \
  return managedPointer.IsValid() ? FCoerceToFText::Value(managedPointer->fieldName) : FText(); \
}

UCLASS()
class UViewingModel : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	FHistoryModel currentModel;

	ADungeonGameModeBase* gm;
};

DECLARE_MULTICAST_DELEGATE_OneParam(TDungeonActionDispatched, const TDungeonAction&)

UCLASS()
class DUNGEON_API ADungeonGameModeBase : public AGameModeBase,
                                         public FStoreConnectedClass<ADungeonGameModeBase, TDungeonAction>
{
	GENERATED_BODY()

	ADungeonGameModeBase(const FObjectInitializer& ObjectInitializer);

public:
	virtual void BeginPlay() override;
	virtual void Tick(float time) override;

	TDungeonActionDispatched DungeonActionDispatched;

	TArray<UViewingModel*> viewingModels;

	UPROPERTY()
	UViewingModel* ViewingModel;
	TSharedPtr<SWindow> ModelViewingWindow;

	UPROPERTY()
	TSubclassOf<AStaticMeshActor> BlockingBorderActorClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> MenuClass;
	UPROPERTY(EditAnywhere)
	UClass* TileShowPrefab;
	UPROPERTY(EditAnywhere)
	FDungeonWorldState Game;
	UPROPERTY(EditAnywhere)
	UClass* UnitActorPrefab;
	UPROPERTY(EditAnywhere)
	UDataTable* UnitTable;
	UPROPERTY(EditAnywhere)
	TMap<TEnumAsByte<ETargetsAvailableId>, FAbilityParams> TargetsColoring;
	UPROPERTY(BlueprintAssignable)
	FCombatActionEvent CombatActionEvent;
	UPROPERTY()
	UDungeonMainWidget* MainWidget;

	TArray<FString> loggingStrings;

	UPROPERTY(VisibleAnywhere)
	USingleSubmitHandler* SingleSubmitHandler;
	
	FTextBlockStyle style;
	TSubclassOf<UDungeonMainWidget> MainWidgetClass;
	TUniquePtr<FDungeonStore> store;
	lager::reader<SIZE_T> interactionReader;

	lager::future Dispatch(TDungeonAction&& unionAction);
	
	template <typename T>
	decltype(auto) Dispatch(T&& x)
	{
		return this->Dispatch(CreateDungeonAction(Forward<T>(x)));
	}
};
