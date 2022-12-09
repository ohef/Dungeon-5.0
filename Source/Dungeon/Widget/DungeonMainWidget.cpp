// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonMainWidget.h"

#include "Algo/Accumulate.h"
#include "Dungeon/Lenses/model.hpp"
#include "Logic/util.h"
#include "Utility/HookFunctor.hpp"

UDungeonMainWidget::UDungeonMainWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

auto GetUnitMenuContext = interactionContextLens
	| unreal_alternative_pipeline<FUnitMenu>;

auto GetUnitIdFromContext = GetUnitMenuContext 
	| map_opt(attr(&FUnitMenu::unitId))
	| value_or(-1);

auto GetMapWidth = attr(&FDungeonWorldState::map) | attr( &FDungeonLogicMap::Width );
auto GetMapHeight = attr(&FDungeonWorldState::map) | attr( &FDungeonLogicMap::Height );

template<typename T>
struct FTypeOps 
{
	static bool IsEqual(const FName& name)
	{
		return T::StaticStruct()->GetFName() == name;
	}

	static FName TypeName() { return T::StaticStruct()->GetFName(); }
};

struct HiddenWidget
{
	TWeakObjectPtr<UWidget> managedWidget;

	HiddenWidget(const TWeakObjectPtr<UWidget>& ManagedWidget)
		: managedWidget(ManagedWidget)
	{
		managedWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	~HiddenWidget(){
		managedWidget->SetVisibility(ESlateVisibility::Visible);
	}
};

struct FDungeonWidgetContextHandler
{
	TWeakObjectPtr<UDungeonMainWidget> dungeonWidget;
	
	FDungeonWidgetContextHandler(UDungeonMainWidget* Widget)
		: dungeonWidget(Widget)
	{
	}

	template <typename TFrom,typename TTo>
	void operator()(TFrom l, TTo r){
		if constexpr (TIsSame<TTo, FUnitMenu>::Value)
		{
			dungeonWidget->UnitActionMenu->SetVisibility(ESlateVisibility::Visible);

			// { dungeonWidget->Attack, dungeonWidget->Wait, dungeonWidget->Move };
			dungeonWidget->Move->SetVisibility(ESlateVisibility::Visible);

			if(r.deactivatedAbilities.Contains("MoveAction")){
				dungeonWidget->Move->SetVisibility(ESlateVisibility::Collapsed);
			}

			if(dungeonWidget->notApplicable["CombatAction"]())
			{
				dungeonWidget->Attack->SetVisibility(ESlateVisibility::Collapsed);
			}
			else
			{
				dungeonWidget->Attack->SetVisibility(ESlateVisibility::Visible);
			}
			
			auto firstSlot = Algo::FindByPredicate(dungeonWidget->UnitActionMenu->GetSlots(),[](UPanelSlot* slot)
			{
				return slot->Content->IsVisible();
			});
			
			(*firstSlot)->Content->SetFocus();
		}
		else if constexpr (TIsSame<TFrom, FUnitMenu>::Value)
		{
			dungeonWidget->UnitActionMenu->SetVisibility(ESlateVisibility::Collapsed);
		}
		else if constexpr (TIsSame<TFrom, FMainMenu>::Value)
		{
			dungeonWidget->MainMapMenu->SetVisibility(ESlateVisibility::Collapsed);
		}
		else if constexpr (TIsSame<TTo, FMainMenu>::Value)
		{
			dungeonWidget->MainMapMenu->SetVisibility(ESlateVisibility::Visible);
			dungeonWidget->MainMapMenu->Units->SetFocus();
		}
		else
		{
			if(!dungeonWidget.IsValid())
				return;
			
			dungeonWidget->UnitActionMenu->SetVisibility(ESlateVisibility::Collapsed);
			dungeonWidget->MainMapMenu->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
};

void UDungeonMainWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);
	UKismetSystemLibrary::PrintString(this,
		"WOWOWOWWOWOWeuastnheoustneoanuheonathuesoanthueoastuhoaenst");
}

bool UDungeonMainWidget::Initialize()
{
	Super::Initialize();

	if (IsDesignTime())
		return true;

	turnStateReader = UseState(		
	lager::lenses::attr(&FDungeonWorldState::TurnState)
		| lager::lenses::attr( &FTurnState::teamId))
	.make();
		
	turnStateReader.watch(THookFunctor<int>(turnStateReader.get(), [this](int teamId)
	                                        {
		                                        TurnNotifierWidget->PlayAnimation(TurnNotifierWidget->FadeIn, 0, 1,
			                                        EUMGSequencePlayMode::PingPong, 1);
	                                        }
	));

	contextCursor = UseState(interactionContextLens);
	contextCursor.bind(TPreviousHookFunctor<TInteractionContext>(contextCursor.get(), [&](auto&& previous, auto&& next)
	{
		Visit(FDungeonWidgetContextHandler( this ), Forward<decltype(previous)>(previous), Forward<decltype(next)>(next));
		return next;
	}));

	MainMapMenu->EndTurn->OnClicked.AddDynamic(this, &UDungeonMainWidget::OnEndTurnClicked);
	Move->OnClicked.AddUniqueDynamic(this, &UDungeonMainWidget::OnMoveClicked);
	Attack->OnClicked.AddUniqueDynamic(this, &UDungeonMainWidget::OnAttackClicked);
	Wait->OnClicked.AddUniqueDynamic(this, &UDungeonMainWidget::OnWaitClicked);

	notApplicable.Add(FName("CombatAction"), [&]
	{
		int unitId = UseViewState(GetUnitIdFromContext);
		auto unitData = *UseViewState(unitDataLens(unitId));
		const auto& state = UseViewState(SimpleCastTo<FDungeonWorldState>);
		auto cursorPosition = UseViewState(attr(&FDungeonWorldState::CursorPosition));

		auto points = manhattanReachablePoints(
			state.map.Width,
			state.map.Height,
			unitData.attackRange,
			cursorPosition);

		for (FIntPoint Point : points)
		{
			const auto& possibleUnit = UseViewState(getUnitAtPointLens(Point));
			if (possibleUnit.IsSet() && UseViewState(unitDataLens(*possibleUnit))->teamId != unitData.teamId)
			{
				return false;
			}
		}

		return true;
	});
	
	return true;
}

void UDungeonMainWidget::OnMoveClicked()
{
	int ViewState = UseViewState(GetUnitIdFromContext);
	StoreDispatch(TDungeonAction(
		TInPlaceType<FSteppedAction>{},
		TStepAction(TInPlaceType<FMoveAction>{}, ViewState),
		TArray{
			TInteractionContext(TInPlaceType<FSelectingUnitAbilityTarget>{})
		}));
}

void UDungeonMainWidget::OnAttackClicked()
{
	StoreDispatch(TDungeonAction(
		TInPlaceType<FSteppedAction>{},
		TStepAction(TInPlaceType<FCombatAction>{}, UseViewState(GetUnitIdFromContext)),
		TArray{
			TInteractionContext(TInPlaceType<FUnitInteraction>{}),
			TInteractionContext(TInPlaceType<FSelectingUnitAbilityTarget>{})
		}));
}

void UDungeonMainWidget::OnWaitClicked()
{
	StoreDispatch(TDungeonAction(TInPlaceType<FWaitAction>{},
	                             UseViewState(GetUnitIdFromContext)
	));
}

void UDungeonMainWidget::OnEndTurnClicked()
{
	StoreDispatch(TDungeonAction(TInPlaceType<FEndTurnAction>{}));
}
