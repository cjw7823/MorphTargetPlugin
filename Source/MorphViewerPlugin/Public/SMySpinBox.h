// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SSpinBox.h"

class MORPHVIEWERPLUGIN_API SMySpinBox : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SMySpinBox) {}
		SLATE_ARGUMENT(FString, MorphTargetName)
		SLATE_ARGUMENT(TSharedPtr< SMySpinBox >, Brother_SpinBox)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void OnValueChanged_SpinBox(float value);

	FString MorphTargetName;

	TWeakPtr< SSpinBox<float> > My_SpinBox;
	TWeakPtr< SMySpinBox > Brother_SpinBox;
	bool bInfiniteLoop = false;
};
