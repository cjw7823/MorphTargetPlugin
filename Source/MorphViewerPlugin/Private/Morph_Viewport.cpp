// Fill out your copyright notice in the Description page of Project Settings.


#include "Morph_Viewport.h"
#include "MorphViewerPlugin.h"
#include "MorphViewportClient.h"
//#include "AdvancedPreviewScene.h"
#include "IPersonaPreviewScene.h"
#include "Persona/Private/AnimationEditorPreviewScene.h"

#include "IPersonaToolkit.h"
#include "IPersonaPreviewScene.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "Animation/MorphTarget.h"

#include "Widgets/Input/SSpinBox.h"
#include "AssetViewerSettings.h"

#define LOCTEXT_NAMESPACE "Morph_Viewport"

SMorph_Viewport::SMorph_Viewport()
	: //PreviewScene(MakeShareable(new FAdvancedPreviewScene(FPreviewScene::ConstructionValues()))),
	Geometry(*(new FGeometry()))
{
	WidgetIds.Init(NULL, NULL);
	CorrectionVec = FVector2D(0, 0);
}

SMorph_Viewport::~SMorph_Viewport()
{
	if (MorphViewportClient.IsValid())
	{
		MorphViewportClient->Viewport = NULL;
	}
}

void SMorph_Viewport::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(MorphObject);
	Collector.AddReferencedObject(PreviewMeshComponent);
}

TSharedRef<class FAnimationEditorPreviewScene> SMorph_Viewport::GetPreviewScene()
{
	return PreviewScene.ToSharedRef();
}

TSharedRef<FEditorViewportClient> SMorph_Viewport::MakeEditorViewportClient()
{
	MorphViewportClient = MakeShareable(new FMorphViewportClient(MorphEditorPtr, PreviewScene.ToSharedRef(), SharedThis(this), MorphObject));
	return MorphViewportClient.ToSharedRef();
}

FReply SMorph_Viewport::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	return FReply::Unhandled();
}

FReply SMorph_Viewport::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	FVector2D Canvas_Geometry = Geometry.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
	FVector2D Viewport_Geometry = GetTickSpaceGeometry().AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());

	//최상단 레이아웃이라면.
	if (IsDrag == false)
	{
		(widgetref)->GetParentWidget()->SetRenderTransform(FSlateRenderTransform(Viewport_Geometry - (ClickedMouseVec)));
		return FReply::Unhandled();
	}
	
	(widgetref)->SetRenderTransform(FSlateRenderTransform(Canvas_Geometry - ClickedMouseVec - CorrectionVec));
	return FReply::Unhandled();
}

void SMorph_Viewport::OnValueChanged_SpinBox(float a)
{
	UE_LOG(MorphViewerPlugin, Log, TEXT("SpinBox_Value : %f"), a);

	PreviewScene->GetPreviewMeshComponent()->SetMorphTarget(FName("Pinnochio"), a);
}

void SMorph_Viewport::Construct(const FArguments& InArgs)
{
	MorphEditorPtr = InArgs._ParentMorphEditor;
	MorphObject = InArgs._ObjectToEdit;
	PersonaToolkit = InArgs._PersonaToolkit_;
	PreviewScene = StaticCastSharedRef<FAnimationEditorPreviewScene>(PersonaToolkit->GetPreviewScene());
	SEditorViewport::Construct(SEditorViewport::FArguments());

	//편집할 오브젝트 설정.
	PreviewMeshComponent_sk = NewObject<USkeletalMeshComponent>(PreviewScene->GetActor(), NAME_None, RF_Transient);

	// auto retClass = LoadObject<USkeletalMesh>(NULL, TEXT("SkeletalMesh'/Game/NewFolder1/SkeletalMeshes/KiteBoyHead_JointsAndBlends.KiteBoyHead_JointsAndBlends'"));
	//PreviewMeshComponent_sk->SetSkeletalMesh(retClass);
	PreviewMeshComponent_sk->SetSkeletalMesh(MorphObject->GetPreviewMesh());

	//FTransform Transform = FTransform::Identity;
	//PreviewScene->AddComponent(PreviewMeshComponent_sk, Transform);
	PreviewMeshComponent_sk->SetSimulatePhysics(true);

	//프리뷰 드레그 위젯 설정.
	ViewportOverlay->AddSlot()
		.VAlign(VAlign_Top)
		.HAlign(HAlign_Left)
		//.Padding(FMargin(10.0f, 10.0f, 10.0f, 10.0f))
		[
			SAssignNew(Canvas, SCanvas)
			.Visibility(EVisibility::Visible)
		];
	Canvas->AddSlot()
		.Size(CANVAS_SLOT_SIZE)
		[
			SAssignNew(DragAndDropVerticalBox, SDragAndDropVerticalBox)
			.Visibility(EVisibility::Visible)
			.RenderTransformPivot(FVector2D(.5, .5))
			.OnCanAcceptDrop(this, &SMorph_Viewport::MyOnCanAcceptDrop)
			.OnAcceptDrop(this, &SMorph_Viewport::MyOnAcceptDrop)
			.OnDragDetected(this, &SMorph_Viewport::MyOnDragDetected)
			.OnDragEnter(this, &SMorph_Viewport::MyOnDragEnter)
			.OnDragLeave(this, &SMorph_Viewport::MyOnDragLeave)
			.OnDrop(this, &SMorph_Viewport::MyOnDrop)
		];

	//ID저장
	WidgetIds.Add(DragAndDropVerticalBox->GetId());
	UE_LOG(MorphViewerPlugin, Error, TEXT("DragAndDropVerticalBox ID : %d"), WidgetIds[0]);
	DragAndDropVerticalBox->AddSlot()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Morph")))
			.ColorAndOpacity(FLinearColor(1, 1, 1, 1))
		];

	TMap<FName, float> MorphCurves = PersonaToolkit->GetPreviewScene()->GetPreviewMeshComponent()->GetMorphTargetCurves();
	TArray<UMorphTarget*>& MorphTargets = MorphObject->GetPreviewMesh()->GetMorphTargets();
	auto MorphTargets2 = PreviewMeshComponent_sk->SkeletalMesh->GetMorphTargets();

	for (int i = 0; i < MorphTargets.Num()+1; i++)
	{
		Canvas->AddSlot()
			.Position(FVector2D(0, CANVAS_POSITION_Y *(i+1)))
			.Size(CANVAS_SLOT_SIZE)
			[
				SAssignNew(VerticalBox, SDragAndDropVerticalBox)
				.Visibility(EVisibility::Visible)
				.RenderTransformPivot(FVector2D(.5, .5))
				.OnCanAcceptDrop(this, &SMorph_Viewport::MyOnCanAcceptDrop2)
				.OnAcceptDrop(this, &SMorph_Viewport::MyOnAcceptDrop)
				.OnDragDetected(this, &SMorph_Viewport::MyOnDragDetected)
				.OnDragEnter(this, &SMorph_Viewport::MyOnDragEnter)
				.OnDragLeave(this, &SMorph_Viewport::MyOnDragLeave)
				.OnDrop(this, &SMorph_Viewport::MyOnDrop)
			];

		WidgetIds.Add(VerticalBox->GetId());
		UE_LOG(MorphViewerPlugin, Error, TEXT("VerticalBox ID : %d"), WidgetIds[i+1]);
		VerticalBox->AddSlot()
			[
				SNew(STextBlock)
				.Text(FText::FromString(MorphTargets[0]->GetName()))
				.ColorAndOpacity(FLinearColor(1, 1, 1, 1))
			];
		VerticalBox->AddSlot()
			[
				SNew(SSpinBox<float>)
				.MinSliderValue(-1.f)
				.MaxSliderValue(1.f)
				.MinValue(-1.0)
				.MaxValue(1.0)
				.OnValueChanged(this, &SMorph_Viewport::OnValueChanged_SpinBox)
			];
	}

}

#undef LOCTEXT_NAMESPACE