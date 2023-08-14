// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/**
 * 
 */
class MORPHVIEWERPLUGIN_API FDetailCustomization : public IDetailCustomization
{
public:
	FDetailCustomization();
	~FDetailCustomization();

	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
