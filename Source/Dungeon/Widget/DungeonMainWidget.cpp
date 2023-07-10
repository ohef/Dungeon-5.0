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
	return *firstSlot;
};

//TODO: I hate this, need to find a better way to define the edges
struct FDungeonWidgetContextHandler
{
	TWeakObjectPtr<UDungeonMainWidget> dungeonWidget;

	FDungeonWidgetContextHandler(UDungeonMainWidget* Widget)
		: dungeonWidget(Widget)
	{
	}

	//TODO: This is literally reconciliation from react...hmm
	template <typename TFrom, typename TTo>
	void operator()(TFrom l, TTo r)
	{
		if constexpr (TIsSame<TTo, FUnitMenu>::Value)
		{
			dungeonWidget->UnitActionMenu->SetVisibility(ESlateVisibility::Visible);

			for (const TFunction<void()>& ActionCheck : dungeonWidget->ActionChecks)
				ActionCheck();

			if constexpr (TNot<TIsSame<TFrom, FUnitMenu>>::Value)
			{
				TArray<UPanelSlot*> PanelSlots = dungeonWidget->UnitActionMenu->GetSlots();
				auto focusedSlot = FocusFirstSlot(PanelSlots);
				dungeonWidget->StoreDispatch(TDungeonAction(TInPlaceType<FFocusChanged>{},
				                                            FName(focusedSlot->Content->GetName())));
			}
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
			if (!dungeonWidget.IsValid())
				return;

			dungeonWidget->UnitActionMenu->SetVisibility(ESlateVisibility::Collapsed);
			dungeonWidget->MainMapMenu->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
};

void UDungeonMainWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);
}

bool UDungeonMainWidget::Initialize()
{
	Super::Initialize();

	if (IsDesignTime())
		return true;

	turnStateReader = UseState(
			lager::lenses::attr(&FDungeonWorldState::TurnState)
			| lager::lenses::attr(&FTurnState::teamId))
		.make();

	turnStateReader.watch([this](int teamId)
		{
			TurnNotifierWidget->PlayAnimation(TurnNotifierWidget->FadeIn, 0, 1,
			                                  EUMGSequencePlayMode::PingPong, 1);
		}
	);

	contextCursor = UseState(interactionContextLens);
	contextCursor.bind(TPreviousHookFunctor<TInteractionContext>(contextCursor.get(), [&](auto&& previous, auto&& next)
	{
		Visit(FDungeonWidgetContextHandler(this), Forward<decltype(previous)>(previous), Forward<decltype(next)>(next));
		return next;
	}));

	FSlateApplication::Get().OnFocusChanging().AddUObject(this, &UDungeonMainWidget::OnFocusChanged);

	MainMapMenu->EndTurn->OnClicked.AddUniqueDynamic(this, &UDungeonMainWidget::OnEndTurnClicked);
	// MainMapMenu->Units->OnClicked.AddUniqueDynamic(this, );
	
	Move->OnClicked.AddUniqueDynamic(this, &UDungeonMainWidget::OnMoveClicked);
	Attack->OnClicked.AddUniqueDynamic(this, &UDungeonMainWidget::OnAttackClicked);
	Wait->OnClicked.AddUniqueDynamic(this, &UDungeonMainWidget::OnWaitClicked);

	auto visibilityCheck = [](const auto& widget, const auto& dontShow)
	{
		return [ dontShow = DUNGEON_FOWARD(dontShow), widget = DUNGEON_FOWARD(widget)]
		{
			dontShow()
				? widget->SetVisibility(ESlateVisibility::Collapsed)
				: widget->SetVisibility(ESlateVisibility::Visible);
		};
	};

	ActionChecks.Add(visibilityCheck(Attack, [&]
	{
		return GetInteractablePositionsAtCursorPosition(UseViewState()).IsEmpty();
	}));

	ActionChecks.Add(visibilityCheck(Move, [&]
	{
		return UseViewState(interactionContextLens | unreal_alternative<FUnitMenu> | ignoreOptional)
		.deactivatedAbilities.Contains("MoveAction");
	}));

	return true;
}

void UDungeonMainWidget::OnFocusChanged(const FFocusEvent& FocusEvent,
                                        const FWeakWidgetPath& OldFocusedWidgetPath,
                                        const TSharedPtr<SWidget>& OldFocusedWidget,
                                        const FWidgetPath& NewFocusedWidgetPath,
                                        const TSharedPtr<SWidget>& NewFocusedWidget)
{
	auto checkWidgets = [&](auto&... Widgets)
	{
		auto checkStuff = [&](auto& Widget)
		{
			if (Widget->TakeWidget() == NewFocusedWidget)
			{
				StoreDispatch(TDungeonAction(TInPlaceType<FFocusChanged>{}, FName(Widget->GetName())));
			}
		};
		( checkStuff(Widgets), ... );
	};

	checkWidgets(Attack, Move, Wait);
}

void UDungeonMainWidget::OnMoveClicked()
{
	int ViewState = UseViewState(GetUnitIdFromContext);
	StoreDispatch(TDungeonAction(
		TInPlaceType<FSteppedAction>{},
		TStepAction(TInPlaceType<FMoveAction>{}, ViewState),
		TArray{
			TInteractionContext(TInPlaceType<FSelectingUnitAbilityTarget>{}, EAbilityId::IdMove)
		}));
}

void UDungeonMainWidget::OnAttackClicked()
{
	StoreDispatch(TDungeonAction(
		TInPlaceType<FSteppedAction>{},
		TStepAction(TInPlaceType<FCombatAction>{}, UseViewState(GetUnitIdFromContext)),
		TArray{
			TInteractionContext(TInPlaceType<FUnitInteraction>{}),
			TInteractionContext(TInPlaceType<FSelectingUnitAbilityTarget>{}, EAbilityId::IdAttack)
		}));
}

void UDungeonMainWidget::OnWaitClicked()
{
	StoreDispatch(FWaitAction{UseViewState(GetUnitIdFromContext)});
	StoreDispatch(FChangeState{TInteractionContext(TInPlaceType<FSelectingUnitContext>())});
}

void UDungeonMainWidget::OnEndTurnClicked()
{
	StoreDispatch(TDungeonAction(TInPlaceType<FEndTurnAction>{}));
}
