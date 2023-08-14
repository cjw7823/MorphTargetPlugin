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

	// FGCObject 인터페이스. U오브젝트가 아니라도 가비지컬렉션 회수 가능.
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	TSharedRef<class FAnimationEditorPreviewScene> GetPreviewScene();

protected:
	// 아래 함수를 구현해 뷰포트 클라이언트를 생성해주어야 한다.
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

private:
	// 부모 에디터, 프리뷰 씬, 뷰포트 클라이언트, 넘겨받은 편집 객체, 페르소나 툴킷
	TWeakPtr<FMorphEditor> MorphEditorPtr;
	TSharedPtr<class FAnimationEditorPreviewScene> PreviewScene;
	TSharedPtr<class FMorphViewportClient> MorphViewportClient;
	UAnimationAsset* MorphObject;
	TSharedPtr<class IPersonaToolkit> PersonaToolkit;

	//캔버스 위젯
	TSharedPtr<SCanvas> Canvas;
	// 오버레이에 사용할 드레그 위젯
	TSharedPtr<SDragAndDropVerticalBox> DragAndDropVerticalBox;
	TSharedPtr<SDragAndDropVerticalBox> VerticalBox;
	//클릭 위치만큼 보정할 벡터
	FVector2D ClickedMouseVec;

	// 프리뷰를 위한 스태틱 메시 컴포넌트.
	class UStaticMeshComponent* PreviewMeshComponent;
	class USkeletalMeshComponent* PreviewMeshComponent_sk;

	/** Editable skeleton */
	TSharedPtr<class IEditableSkeleton> EditableSkeleton;

	//드레그 SWidget을 만들기 위한 함수들.
public:
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	//전체 위젯 이동
	TOptional<SDragAndDropVerticalBox::EItemDropZone> MyOnCanAcceptDrop(const FDragDropEvent& DragDropEvent, SDragAndDropVerticalBox::EItemDropZone zone, SVerticalBox::FSlot* slot)
	{
		FVector2D vec = GetTickSpaceGeometry().AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());

		widgetref->GetParentWidget()->SetRenderTransform(FSlateRenderTransform((vec - ClickedMouseVec)));
		return SDragAndDropVerticalBox::EItemDropZone::AboveItem;
	}

	//개별 위젯 이동
	TOptional<SDragAndDropVerticalBox::EItemDropZone> MyOnCanAcceptDrop2(const FDragDropEvent& DragDropEvent, SDragAndDropVerticalBox::EItemDropZone zone, SVerticalBox::FSlot* slot)
	{
		FVector2D vec = Geometry.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());

		widgetref->SetRenderTransform(FSlateRenderTransform((vec - ClickedMouseVec - CorrectionVec)));
		return SDragAndDropVerticalBox::EItemDropZone::AboveItem;
	}

	FReply MyOnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, int32 i , SVerticalBox::FSlot* slot)
	{
		//뷰포트의 지오메트리.
		Geometry = Canvas->GetTickSpaceGeometry();

		widgetref = (slot->GetWidget()->GetParentWidget());
		UE_LOG(MorphViewerPlugin, Error, TEXT("slot ID : %d"), widgetref->GetId());

		//드레그 위젯 ID체크. 0이면 최상단 레이아웃.
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
		//일반적으로 동작 안함.
		return FReply::Handled();
	}

	FGeometry& Geometry;
	TSharedPtr<SWidget> widgetref;
	TArray<uint64> WidgetIds;
	bool IsDrag = true;
	FVector2D CorrectionVec;

	void OnValueChanged_SpinBox(float a);
};
