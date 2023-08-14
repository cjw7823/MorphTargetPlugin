// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class MORPHVIEWERPLUGIN_API SMyButton : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SMyButton) {}
		SLATE_ARGUMENT(TWeakPtr<SWidget>, RemovedObj)
		SLATE_ARGUMENT(FString, Mode)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	FReply OnClicked_SMyButton();
	FReply MyOnClicked_Reset();

	TWeakPtr<SWidget> widgetRef;
};
