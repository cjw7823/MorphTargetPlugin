// Fill out your copyright notice in the Description page of Project Settings.


#include "SDockingAreaCanvas.h"
#include "MorphViewerPluginCommands.h"
#include "MorphViewerPlugin.h"
#include "SMyCanvas.h"
#include "Persona/Private/SAnimationEditorViewport.h"
#include "SMySpinBox.h"
#include "Animation/MorphTarget.h"
#include "UnrealEd/Private/Toolkits/SStandaloneAssetEditorToolkitHost.h"
#include "SMyButton.h"

FReply SDockingAreaCanvas::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	FVector2D Canvas_Geometry = funclass::GetInstance()->DockingAreaCanvas->GetTickSpaceGeometry().AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());

	funclass::GetInstance()->DraggedWidgetShadow_Tab->SetRenderTransform(FSlateRenderTransform(Canvas_Geometry - CANVAS_SLOT_SIZE / 2));

	return FReply::Unhandled();
}

FReply SDockingAreaCanvas::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	UE_LOG(MorphViewerPlugin, Error, TEXT("SDockingAreaCanvas OnDragDetected"));
	return FReply::Unhandled();
}

FReply SDockingAreaCanvas::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	//DraggedWidgetShadow_Tab 제거.
	funclass::GetInstance()->DockingAreaCanvas->RemoveSlot(funclass::GetInstance()->DraggedWidgetShadow_Tab.ToSharedRef());

	//DockingAreaCanvas의 드레그 감지 OFF.
	funclass::GetInstance()->DockingAreaCanvas->SetVisibility(EVisibility::SelfHitTestInvisible);

	//놓은 위치가 뷰포트라면 위젯 생성.
	auto size = FMorphViewerPluginActions::ViewportOverlay.Pin()->GetParentWidget()->GetTickSpaceGeometry().GetLocalSize();
	auto position = funclass::GetInstance()->ViewportCanvas->GetTickSpaceGeometry().AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
	auto position2 = funclass::GetInstance()->MorphLayoutCanvas->GetTickSpaceGeometry().AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());

	if (position.X >=0 && position.Y >= 0 &&
		position.X <= size.X && position.Y <= size.Y)
	{
		//UE_LOG(MorphViewerPlugin, Warning, TEXT("in viewport"));

		//모프타깃 이름이 동일한 스핀 박스는 생성하지 않는다.
		for(auto spinbox : funclass::GetInstance()->ViewportSpinboxList)
		{
			if (spinbox->MorphTargetName == funclass::GetInstance()->ChooseSpinbox->MorphTargetName)
			{
				UE_LOG(MorphViewerPlugin, Warning, TEXT("same morph target name"));
				return FReply::Unhandled().EndDragDrop();
			}
		}

		TSharedPtr<SDragAndDropVerticalBox> tempwidget;
		funclass::GetInstance()->MorphLayoutCanvas->AddSlot()
			.Size(CANVAS_SLOT_SIZE)
			[
				SAssignNew(tempwidget, SDragAndDropVerticalBox)
				.Visibility(EVisibility::Visible)
				.RenderTransformPivot(FVector2D(.5, .5))
				.OnDragDetected(funclass::GetInstance().Get(), &funclass::MyOnDragDetected_Viewport)
				.OnCanAcceptDrop(funclass::GetInstance().Get(), &funclass::MyOnCanAcceptDrop_Viewport)
				.OnAcceptDrop(funclass::GetInstance().Get(), &funclass::MyOnAcceptDrop_Viewport)
				.OnDragLeave(funclass::GetInstance().Get(), &funclass::MyOnDragLeave_Viewport)
				//.RenderTransform(position2 - CANVAS_SLOT_SIZE/2)
			];

		tempwidget->SetRenderTransform(position2 - CANVAS_SLOT_SIZE / 2);

		tempwidget->AddSlot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(80.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(funclass::GetInstance()->ChooseSpinbox->MorphTargetName))
					.ColorAndOpacity(FLinearColor(1, 1, 1, 1))					
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SMyButton)
					.RemovedObj(tempwidget)
					.Mode(FString("Close"))
				]
			];

		TSharedPtr<SMySpinBox> spinbox;
		tempwidget->AddSlot()
			[
				SAssignNew(spinbox, SMySpinBox)
				.MorphTargetName(funclass::GetInstance()->ChooseSpinbox->MorphTargetName)
				.Brother_SpinBox(funclass::GetInstance()->ChooseSpinbox)
			];

		funclass::GetInstance()->ViewportSpinboxList.Add(spinbox);
		funclass::GetInstance()->ChooseSpinbox->Brother_SpinBox = spinbox;

		funclass::GetInstance()->AddCurveData();
	}

	return FReply::Unhandled().EndDragDrop();
}

void SDockingAreaCanvas::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	//함수가 호출 될 때, 비정상적 호출인지 판별.
	auto temp = FMorphViewerPluginActions::StandaloneAssetEditorToolkitHost.Pin();
	auto size = temp->GetParentWidget()->GetTickSpaceGeometry().GetLocalSize();
	auto position = temp->GetTickSpaceGeometry().AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());

	//뷰포트 오버레이의 밖에서 호출된다면
	if (position.X <= 0 || position.Y <= 0 ||
		position.X >= size.X || position.Y >= size.Y)
	{
		//DockingAreaCanvas의 드레그 감지 OFF.
		funclass::GetInstance()->DockingAreaCanvas->SetVisibility(EVisibility::SelfHitTestInvisible);
		funclass::GetInstance()->DockingAreaCanvas->RemoveSlot(funclass::GetInstance()->DraggedWidgetShadow_Tab.ToSharedRef());
	}
}
