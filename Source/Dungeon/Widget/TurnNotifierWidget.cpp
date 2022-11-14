// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnNotifierWidget.h"

bool UTurnNotifierWidget::Initialize()
{
  Super::Initialize();
  return true;
}

void UTurnNotifierWidget::PlayMyAnimation(){
  PlayAnimation(FadeIn, 0, 1, EUMGSequencePlayMode::Forward, 1);
}
