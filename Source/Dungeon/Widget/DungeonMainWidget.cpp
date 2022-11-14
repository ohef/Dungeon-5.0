// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonMainWidget.h"

#include "Dungeon/DungeonGameModeBase.h"
#include "Dungeon/Lenses/model.hpp"
#include "lager/lenses/tuple.hpp"
#include "Utility/HookFunctor.hpp"
#include "Utility/ContextSwitchVisitor.hpp"

UDungeonMainWidget::UDungeonMainWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

template <typename TCheck, typename TArg>
using TEnableIfNotType = typename TEnableIf<TNot<TIsSame<TCheck, typename TDecay<TArg>::Type>>::Value, typename TDecay<
	                                            TArg>::Type>::Type;

STRUCT_CONTEXT_SWITCH_VISITOR(FContextHandler)
{
	STRUCT_CONTEXT_SWITCH_VISITOR_BODY(FContextHandler)
	
	UDungeonMainWidget* gm;
	
	FContextHandler(UDungeonMainWidget* Gm)
		: gm(Gm)
	{
	}
	
	template <class Tx>
	void transition(FMainMenu previouss, Tx nextt)
	{
		gm->MainMapMenu->SetVisibility(ESlateVisibility::Collapsed);
	}

	template <class Tx>
	void transition(Tx nextt, FMainMenu previouss)
	{
		gm->MainMapMenu->SetVisibility(ESlateVisibility::Visible);
	}
};

bool UDungeonMainWidget::Initialize()
{
	Super::Initialize();

	if (IsDesignTime())
		return true;

	const auto& DungeonWorldState = lager::view(worldStoreLens, GetWorld());
	turnStateReader = DungeonWorldState->zoom(
		SimpleCastTo<FDungeonWorldState> | lager::lenses::attr(&FDungeonWorldState::TurnState) | lager::lenses::attr(
			&FTurnState::teamId)).make();
	TurnNotifierWidget->SetVisibility(ESlateVisibility::Collapsed);
	turnStateReader.watch(THookFunctor<int>(turnStateReader.get(), [this](int teamId)
	                                        {
		                                        TurnNotifierWidget->SetVisibility(ESlateVisibility::Visible);
		                                        TurnNotifierWidget->PlayAnimation(TurnNotifierWidget->FadeIn, 0, 1,
			                                        EUMGSequencePlayMode::PingPong, 1);
	                                        }
	));

	contextCursor = DungeonWorldState->zoom(interactionContextLens);
	contextCursor.bind(TPreviousHookFunctor<TInteractionContext>(contextCursor.get(), [&](auto&& previous, auto&& next)
	{
		Visit(FContextHandler( this ), Forward<decltype(previous)>(previous), Forward<decltype(next)>(next));
		return next;
	}));

	this->MainMapMenu->EndTurn->OnClicked.AddDynamic(this, &UDungeonMainWidget::OnEndTurnClicked);

	return true;
}

void UDungeonMainWidget::OnEndTurnClicked()
{
	StoreDispatch(TAction(TInPlaceType<FEndTurnAction>{}));
}