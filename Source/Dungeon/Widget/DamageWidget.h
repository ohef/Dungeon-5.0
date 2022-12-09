// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "DamageWidget.generated.h"

/**
 * 
 */
UCLASS()
class DUNGEON_API UDamageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TWeakObjectPtr<UTextBlock> DamageDone;

	UPROPERTY(meta=(BindWidgetAnim), BlueprintReadOnly, Transient)
	TWeakObjectPtr<UWidgetAnimation> DamageDissappear;
};
