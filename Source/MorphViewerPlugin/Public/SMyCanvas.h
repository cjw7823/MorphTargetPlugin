// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCanvas.h"

class MORPHVIEWERPLUGIN_API SMyCanvas : public SCanvas
{
public:
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
};
