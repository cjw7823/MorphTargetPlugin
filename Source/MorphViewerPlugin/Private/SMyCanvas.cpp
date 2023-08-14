// Fill out your copyright notice in the Description page of Project Settings.


#include "SMyCanvas.h"
#include "MorphViewerPluginCommands.h"
#include "MorphViewerPlugin.h"

FReply SMyCanvas::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	//다른 탭 등으로 OnDragOver가 호출되는 것을 방지.
	if (!funclass::GetInstance()->DraggedWidgetRef_in_Viewport.IsValid())
		return FReply::Unhandled();

	if (funclass::GetInstance()->bDraggedWidget_is_Title)
	{
		FVector2D Canvas_Geometry = funclass::GetInstance()->ViewportCanvas->GetTickSpaceGeometry().AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
		funclass::GetInstance()->MorphLayoutCanvas->SetRenderTransform(FSlateRenderTransform(Canvas_Geometry - funclass::GetInstance()->ClickedMouseVec));
	}
	else
	{
		FVector2D Canvas_Geometry = funclass::GetInstance()->MorphLayoutCanvas->GetTickSpaceGeometry().AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
		funclass::GetInstance()->DraggedWidgetRef_in_Viewport->SetRenderTransform(FSlateRenderTransform(Canvas_Geometry - funclass::GetInstance()->ClickedMouseVec));
	}

	return FReply::Unhandled();
}