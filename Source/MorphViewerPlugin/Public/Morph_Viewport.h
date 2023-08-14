// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EngineMinimal.h"
#include "SlateFwd.h"
#include "UObject/GCObject.h"
#include "SEditorViewport.h"
#include "SCommonEditorViewportToolbarBase.h"

#include "MorphViewerPlugin.h"

#include "Widgets/SCanvas.h"

#define CANVAS_SLOT_SIZE FVector2D(100, 40)
#define CANVAS_POSITION_Y 40

class FMorphEditor;

class SMorph_Viewport : public SEditorViewport, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SMorph_Viewport) {}
		SLATE_ARGUMENT(TWeakPtr<FMorphEditor>, ParentMorphEditor)
		SLATE_ARGUMENT(UAnimationAsset*, ObjectToEdit)
		SLATE_ARGUMENT(TSharedPtr<class IPersonaToolkit>, PersonaToolkit_)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	SMorph_Viewport();
	~SMorph_Viewport();

	// FGCObject �������̽�. U������Ʈ�� �ƴ϶� �������÷��� ȸ�� ����.
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	TSharedRef<class FAnimationEditorPreviewScene> GetPreviewScene();

protected:
	// �Ʒ� �Լ��� ������ ����Ʈ Ŭ���̾�Ʈ�� �������־�� �Ѵ�.
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

private:
	// �θ� ������, ������ ��, ����Ʈ Ŭ���̾�Ʈ, �Ѱܹ��� ���� ��ü, �丣�ҳ� ��Ŷ
	TWeakPtr<FMorphEditor> MorphEditorPtr;
	TSharedPtr<class FAnimationEditorPreviewScene> PreviewScene;
	TSharedPtr<class FMorphViewportClient> MorphViewportClient;
	UAnimationAsset* MorphObject;
	TSharedPtr<class IPersonaToolkit> PersonaToolkit;

	//ĵ���� ����
	TSharedPtr<SCanvas> Canvas;
	// �������̿� ����� �巹�� ����
	TSharedPtr<SDragAndDropVerticalBox> DragAndDropVerticalBox;
	TSharedPtr<SDragAndDropVerticalBox> VerticalBox;
	//Ŭ�� ��ġ��ŭ ������ ����
	FVector2D ClickedMouseVec;

	// �����並 ���� ����ƽ �޽� ������Ʈ.
	class UStaticMeshComponent* PreviewMeshComponent;
	class USkeletalMeshComponent* PreviewMeshComponent_sk;

	/** Editable skeleton */
	TSharedPtr<class IEditableSkeleton> EditableSkeleton;

	//�巹�� SWidget�� ����� ���� �Լ���.
public:
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	//��ü ���� �̵�
	TOptional<SDragAndDropVerticalBox::EItemDropZone> MyOnCanAcceptDrop(const FDragDropEvent& DragDropEvent, SDragAndDropVerticalBox::EItemDropZone zone, SVerticalBox::FSlot* slot)
	{
		FVector2D vec = GetTickSpaceGeometry().AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());

		widgetref->GetParentWidget()->SetRenderTransform(FSlateRenderTransform((vec - ClickedMouseVec)));
		return SDragAndDropVerticalBox::EItemDropZone::AboveItem;
	}

	//���� ���� �̵�
	TOptional<SDragAndDropVerticalBox::EItemDropZone> MyOnCanAcceptDrop2(const FDragDropEvent& DragDropEvent, SDragAndDropVerticalBox::EItemDropZone zone, SVerticalBox::FSlot* slot)
	{
		FVector2D vec = Geometry.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());

		widgetref->SetRenderTransform(FSlateRenderTransform((vec - ClickedMouseVec - CorrectionVec)));
		return SDragAndDropVerticalBox::EItemDropZone::AboveItem;
	}

	FReply MyOnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, int32 i , SVerticalBox::FSlot* slot)
	{
		//����Ʈ�� ������Ʈ��.
		Geometry = Canvas->GetTickSpaceGeometry();

		widgetref = (slot->GetWidget()->GetParentWidget());
		UE_LOG(MorphViewerPlugin, Error, TEXT("slot ID : %d"), widgetref->GetId());

		//�巹�� ���� IDüũ. 0�̸� �ֻ�� ���̾ƿ�.
		int32 index = WidgetIds.Find(widgetref->GetId());
		if (index == 0)
			IsDrag = false;
		else
		{
			IsDrag = true;
			CorrectionVec.Y = index * CANVAS_POSITION_Y;
		}

		ClickedMouseVec = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		//UE_LOG(MorphViewerPlugin, Error, TEXT("ClickedMouseVec : %s"), *(ClickedMouseVec.ToString()));

		auto Operation = MakeShared<FDragAndDropVerticalBoxOp>(*(new FDragAndDropVerticalBoxOp()));
		return FReply::Handled().BeginDragDrop(Operation);
	}
	
	FReply MyOnAcceptDrop(const FDragDropEvent& DragDropEvent, SDragAndDropVerticalBox::EItemDropZone zone, int32 i, SVerticalBox::FSlot* slot)
	{
		return FReply::Handled().EndDragDrop();
	}

	void MyOnDragEnter(FDragDropEvent const& enter)
	{
		//UE_LOG(MorphViewerPlugin, Error, TEXT("OnDragEnter!!!!"));
	}

	void MyOnDragLeave(FDragDropEvent const& leave)
	{
		//UE_LOG(MorphViewerPlugin, Error, TEXT("OnDragLeave!!!!!"));
	}

	FReply MyOnDrop(FDragDropEvent const& drop)
	{
		//�Ϲ������� ���� ����.
		return FReply::Handled();
	}

	FGeometry& Geometry;
	TSharedPtr<SWidget> widgetref;
	TArray<uint64> WidgetIds;
	bool IsDrag = true;
	FVector2D CorrectionVec;

	void OnValueChanged_SpinBox(float a);
};
