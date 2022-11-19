﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonMainWidget.h"

#include "Dungeon/DungeonGameModeBase.h"
#include "Dungeon/Lenses/model.hpp"
#include "lager/lenses/tuple.hpp"
#include "Utility/HookFunctor.hpp"
#include "Utility/ContextSwitchVisitor.hpp"

UDungeonMainWidget::UDungeonMainWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

struct FDungeonWidgetContextHandler
{
	TWeakObjectPtr<UDungeonMainWidget> dungeonWidget;
	
	FDungeonWidgetContextHandler(UDungeonMainWidget* Gm)
		: dungeonWidget(Gm)
	{
	}

	template <typename TFrom,typename TTo>
	void operator()(TFrom l, TTo r){
		if constexpr (TIsSame<TFrom, FUnitMenu>::Value)
		{
			dungeonWidget->UnitActionMenu->SetVisibility(ESlateVisibility::Collapsed);
		}
		else if constexpr (TIsSame<TTo, FUnitMenu>::Value)
		{
			dungeonWidget->UnitActionMenu->SetVisibility(ESlateVisibility::Visible);
		}
		else if constexpr (TIsSame<TFrom, FMainMenu>::Value)
		{
			dungeonWidget->MainMapMenu->SetVisibility(ESlateVisibility::Collapsed);
		}
		else if constexpr (TIsSame<TTo, FMainMenu>::Value)
		{
			dungeonWidget->MainMapMenu->SetVisibility(ESlateVisibility::Visible);
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
		                                        TurnNotifierWidget->SetVisibility(ESlateVisibility::Visible);
		                                        TurnNotifierWidget->PlayAnimation(TurnNotifierWidget->FadeIn, 0, 1,
			                                        EUMGSequencePlayMode::PingPong, 1);
	                                        }
	));
	
	TurnNotifierWidget->SetVisibility(ESlateVisibility::Collapsed);

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
	
	return true;
}

void UDungeonMainWidget::OnMoveClicked()
{
	StoreDispatch(TDungeonAction(
		TInPlaceType<FInteractAction>{},
		TStepAction(TInPlaceType<FMoveAction>{},
		            UseViewState(interactionContextLens | unreal_alternative<FUnitMenu> | ignoreOptional).unitId
			),
		TArray{TInteractionContext(TInPlaceType<FSelectingUnitAbilityTarget>{})}
		));
}

void UDungeonMainWidget::OnAttackClicked()
{
	StoreDispatch(TDungeonAction(
		TInPlaceType<FInteractAction>{},
		TStepAction(TInPlaceType<FCombatAction>{}),
		TArray{
			TInteractionContext(TInPlaceType<FSelectingUnitAbilityTarget>{}),
			TInteractionContext(TInPlaceType<FUnitInteraction>{})
		}));
}

void UDungeonMainWidget::OnWaitClicked()
{
	StoreDispatch(TDungeonAction( TInPlaceType<FWaitAction>{},
		UseViewState(interactionContextLens | unreal_alternative<FUnitMenu> | ignoreOptional ).unitId
	));
}

void UDungeonMainWidget::OnEndTurnClicked()
{
	StoreDispatch(TDungeonAction(TInPlaceType<FEndTurnAction>{}));
}