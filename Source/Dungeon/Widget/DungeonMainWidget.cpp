// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonMainWidget.h"

#include "Algo/Accumulate.h"
#include "Dungeon/Lenses/model.hpp"
#include "Logic/StateQueries.hpp"
#include "Utility/HookFunctor.hpp"

UDungeonMainWidget::UDungeonMainWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

const auto FocusFirstSlot = [](TArray<UPanelSlot*> slots)
{
	auto firstSlot = Algo::FindByPredicate(slots, [](UPanelSlot* slot)
	{
		return slot->Content->IsVisible();
	});
	(*firstSlot)->Content->SetFocus();
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

			TArray<UPanelSlot*> PanelSlots = dungeonWidget->UnitActionMenu->GetSlots();
			FocusFirstSlot(PanelSlots);
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
		return GetInteractablePositions(UseViewState()).IsEmpty();
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
			TInteractionContext(TInPlaceType<FSelectingUnitAbilityTarget>{},  EAbilityId::IdMove)
		}));
}

void UDungeonMainWidget::OnAttackClicked()
{
	StoreDispatch(TDungeonAction(
		TInPlaceType<FSteppedAction>{},
		TStepAction(TInPlaceType<FCombatAction>{}, UseViewState(GetUnitIdFromContext)),
		TArray{
			TInteractionContext(TInPlaceType<FUnitInteraction>{}),
			TInteractionContext(TInPlaceType<FSelectingUnitAbilityTarget>{}, EAbilityId::IdAttack )
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
