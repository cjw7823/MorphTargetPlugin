// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "MorphViewerPluginStyle.h"

#define CANVAS_SLOT_SIZE FVector2D(100, 40)
#define CANVAS_POSITION_Y 40

class MORPHVIEWERPLUGIN_API FMorphViewerPluginCommands : public TCommands<FMorphViewerPluginCommands>
{
public:
	FMorphViewerPluginCommands()
		: TCommands<FMorphViewerPluginCommands>(TEXT("MorphViewerPlugin"),
			NSLOCTEXT("MorphViewerPlugin", "MorphViewerPlugin", "MorphViewerPlugin"),
			NAME_None, FMorphViewerPluginStyle::GetStyleSetName())
	{
	}

	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > Command1;
	TSharedPtr< FUICommandInfo > Command2;
};



class FMorphViewerPluginActions
{
public:
	static void Action1();

	//새로운 에셋을 만들어 저장하는 기능. 엔진 지원기능으로 개발 중단.
	static void Action2();

private:
	//스택 용량부족으로 여러번 나눠 계층을 따라간다. 4.27기준 계층구조. 최우선 호출 필요.
	static void FindViewportOverlay();

	static TWeakPtr<class SStandaloneAssetEditorToolkitHost> StandaloneAssetEditorToolkitHost;
	static TWeakPtr<class SAnimationEditorViewportTabBody> AnimationEditorViewportTabBody;
	static TWeakPtr<SOverlay> ViewportOverlay;
	static TWeakPtr<SOverlay> DockingAreaOverlay;

	static TWeakPtr<class MyAnimationEditor > NewAnimEditor;

	friend class funclass;
	friend class SDockingAreaCanvas;
	friend class SMySpinBox;
	friend class SMyButton;
};


//FGlobalTabmanager에 스포너로 등록할 콜백 객체. 싱글톤
class funclass : public TSharedFromThis<funclass>
{
public:
	//탭 생성
	TSharedRef<SDockTab> OnSpawnMyTab();
    
	//드래그 관련 함수_뷰포트
	FReply MyOnDragDetected_Viewport_Title(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, int32 i, SVerticalBox::FSlot* slot);
	FReply MyOnDragDetected_Viewport(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, int32 i, SVerticalBox::FSlot* slot);
	TOptional<SDragAndDropVerticalBox::EItemDropZone> MyOnCanAcceptDrop_Viewport_Title(const FDragDropEvent& DragDropEvent, SDragAndDropVerticalBox::EItemDropZone zone, SVerticalBox::FSlot* slot);
	TOptional<SDragAndDropVerticalBox::EItemDropZone> MyOnCanAcceptDrop_Viewport(const FDragDropEvent& DragDropEvent, SDragAndDropVerticalBox::EItemDropZone zone, SVerticalBox::FSlot* slot);
	FReply MyOnAcceptDrop_Viewport(const FDragDropEvent& DragDropEvent, SDragAndDropVerticalBox::EItemDropZone zone, int32 i, SVerticalBox::FSlot* slot);
	void MyOnDragLeave_Viewport(FDragDropEvent const& DragDropEvent);

	//드래그 관련 함수_탭
	FReply MyOnDragDetected_Tab(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, int32 i, SVerticalBox::FSlot* slot);

	//Curve 데이터 추가.
	void AddCurveData();

	//싱글톤 Getter
	static TSharedPtr<funclass> GetInstance();
private:
	//숨긴 생성자
	funclass() {};

	UPROPERTY()
	static TSharedPtr<funclass> Instance;

public:
	//클릭 위치 보정 벡터
	FVector2D ClickedMouseVec;

	//Dragged Widget
	TSharedPtr<SWidget> DraggedWidgetRef_in_Viewport;
	TSharedPtr<class SDockingAreaCanvas>  DraggedWidgetShadow_Tab;
	bool bDraggedWidget_is_Title = true;

	//DraggedWidgetShadow_Tab이 붙을 캔버스
	TSharedPtr<class SDockingAreaCanvas> DockingAreaCanvas = nullptr;
	//뷰포트의 캔버스 위젯
	TSharedPtr<class SMyCanvas> ViewportCanvas = nullptr;
	//모프타깃 레이아웃이 배치 될 캔버스
	TSharedPtr<class SCanvas> MorphLayoutCanvas = nullptr;
	//프리뷰 씬
	TSharedPtr<class FAnimationEditorPreviewScene> PreviewScene;

	//선택된 스핀 박스
	TSharedPtr<class SMySpinBox> ChooseSpinbox;
	//배치된 스핀박스 리스트
	TArray<TSharedPtr<class SMySpinBox>> ViewportSpinboxList;
	//커스텀 탭의 스핀박스 리스트
	TArray<TSharedPtr<class SMySpinBox>> SpinboxList;
};