// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnNotifierWidget.h"

bool UTurnNotifierWidget::Initialize()
{
	Super::Initialize();


	if (!GetWorld()->template GetAuthGameMode<ADungeonGameModeBase>())
	{
		CurrentTurn = lager::make_state(FText::FromString("Player {0} Phase"));
		return true;
	}

	CurrentTurn =
		this->UseStoreNode().zoom(
			lager::lenses::attr(&FDungeonWorldState::TurnState)
			| lager::lenses::attr(&FTurnState::teamId)).map(
			[](auto&& x) -> FText
			{
				return FText::Format(FTextFormat::FromString("Player {0} Phase"), x);
			});

	return true;
}

void UTurnNotifierWidget::PlayMyAnimation()
{
	PlayAnimation(FadeIn, 0, 1, EUMGSequencePlayMode::Forward, 1);
}