// Fill out your copyright notice in the Description page of Project Settings.


#include "MorphViewerPluginCommands.h"
#include "MorphViewerPlugin.h"
#include "Persona/Private/SAnimationEditorViewport.h"

#include "Animation/MorphTarget.h"

#include "Widgets/Layout/SScrollBox.h"
#include "SMySpinBox.h"
#include "Styling/SlateTypes.h"
#include "SMyCanvas.h"
#include "SDockingAreaCanvas.h"

#include "Widgets/SOverlay.h"
#include "MorphEditor.h"

#include "PropertyEditorModule.h"
#include "IPropertyRowGenerator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "IDetailTreeNode.h"

#include "Persona/Private/SAnimSequenceCurveEditor.h"
#include "CurveEditor/Public/CurveModel.h"
#include "CurveEditor/Public/Views/SInteractiveCurveEditorView.h"

#include "MyAnimationEditor.h"
#include "UnrealEd/Private/Toolkits/SStandaloneAssetEditorToolkitHost.h"
#include "MyAnimationEditor.h"
#include "Persona/Private/SSequenceEditor.h"
#include "SMyButton.h"

#include "AssetToolsModule.h"
#include "PackageTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorFramework/AssetImportData.h"

#define LOCTEXT_NAMESPACE "MorphCommand"

void FMorphViewerPluginCommands::RegisterCommands()
{
	UI_COMMAND(Command1, "MorphTargetPlugin", "뷰포트에 모프타깃 컨트롤을 배치합니다.", EUserInterfaceActionType::Button, FInputGesture());
    UI_COMMAND(Command2, "TEST_BUTTON", "테스트 버튼", EUserInterfaceActionType::Button, FInputGesture());
}

TSharedPtr<funclass> funclass::Instance = nullptr;
TWeakPtr<SStandaloneAssetEditorToolkitHost> FMorphViewerPluginActions::StandaloneAssetEditorToolkitHost = nullptr;
TWeakPtr<SAnimationEditorViewportTabBody> FMorphViewerPluginActions::AnimationEditorViewportTabBody = nullptr;
TWeakPtr<SOverlay> FMorphViewerPluginActions::ViewportOverlay = nullptr;
TWeakPtr<SOverlay> FMorphViewerPluginActions::DockingAreaOverlay = nullptr;
TWeakPtr<MyAnimationEditor> FMorphViewerPluginActions::NewAnimEditor = nullptr;

void FMorphViewerPluginActions::Action1()
{
    if (MyAnimationEditor::Count == 0)
    {
        FindViewportOverlay();

        //커스텀 애님 에디터
        UAnimationAsset* asset = AnimationEditorViewportTabBody.Pin()->GetPreviewScene()->GetPreviewAnimationAsset();
        TSharedPtr<MyAnimationEditor> TempEditor = MakeShareable<>(new MyAnimationEditor());
        NewAnimEditor = TempEditor;
        NewAnimEditor.Pin()->InitAnimationEditor(EToolkitMode::Standalone, TSharedPtr<IToolkitHost>(), asset);

        //기존 애님 애디터 닫기
        if (StandaloneAssetEditorToolkitHost.IsValid())
            StandaloneAssetEditorToolkitHost.Pin()->GetTabManager()->GetOwnerTab()->RequestCloseTab();

        //커스텀 애님 에디터의 포인터로 갱신.
        FindViewportOverlay();

        //모프타깃 레이아웃을 뷰포트에 생성.
        TSharedPtr<SCanvas> temp_canvas;
        ViewportOverlay.Pin()->AddSlot()
            .VAlign(VAlign_Top)
            .HAlign(HAlign_Left)
            [
                SAssignNew(temp_canvas, SCanvas)
                .Visibility(EVisibility::Visible)
            ];

        temp_canvas->AddSlot()
            .VAlign(VAlign_Top)
            .HAlign(HAlign_Left)
            .Size(FVector2D(1920, 1080))
            [
                SAssignNew(funclass::GetInstance()->ViewportCanvas, SMyCanvas)
                .Visibility(EVisibility::SelfHitTestInvisible)
            ];

        funclass::GetInstance()->ViewportCanvas->AddSlot()
            //.Position(FVector2D(300, 100))
            .Size(CANVAS_SLOT_SIZE)
            [
                SAssignNew(funclass::GetInstance()->MorphLayoutCanvas, SCanvas)
            ];

        funclass::GetInstance()->MorphLayoutCanvas->SetRenderTransform(FVector2D(300, 100));

        TSharedPtr<SDragAndDropVerticalBox> temp_widget;
        funclass::GetInstance()->MorphLayoutCanvas->AddSlot()
            .Size(CANVAS_SLOT_SIZE)
            [
                SAssignNew(temp_widget, SDragAndDropVerticalBox)
                .Visibility(EVisibility::Visible)
                //.RenderTransformPivot(FVector2D(.5, .5))
                .OnDragDetected(funclass::GetInstance().Get(), &funclass::MyOnDragDetected_Viewport_Title)
                .OnCanAcceptDrop(funclass::GetInstance().Get(), &funclass::MyOnCanAcceptDrop_Viewport_Title)
                .OnAcceptDrop(funclass::GetInstance().Get(), &funclass::MyOnAcceptDrop_Viewport)
                .OnDragLeave(funclass::GetInstance().Get(), &funclass::MyOnDragLeave_Viewport)
            ];

        temp_widget->AddSlot()
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Morph")))
                .Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 20, EFontHinting::Default, FFontOutlineSettings(1)))
                .ColorAndOpacity(FLinearColor(1, 1, 0, 1))
            ];
    }

    //커스텀 애님 애디터에 커스텀 탭이 없다면 생성.
    if (!NewAnimEditor.Pin()->GetTabManager()->FindExistingLiveTab(AnimationEditorTabs::MorphTargetListTab))
    {
        //커스텀 탭 생성.
        auto tabRef = funclass::GetInstance()->OnSpawnMyTab();
        NewAnimEditor.Pin()->GetTabManager()->InsertNewDocumentTab(AnimationEditorTabs::MorphTargetListTab, FTabManager::ESearchPreference::RequireClosedTab, tabRef);
    }
}

void FMorphViewerPluginActions::Action2()
{
    /*
    FString AssetName = "11111111111111111";
    // 프로젝트 콘텐츠 폴더 경로
    FString ContentDir = FPaths::ProjectContentDir();

    // 에셋을 저장할 패키지 경로 설정
    FString PackageFullName = FString::Printf(TEXT("%s%s"), TEXT("/Game/"), *AssetName);

    // 에셋 생성
    UPackage* Package = CreatePackage(nullptr, *PackageFullName);
    Package->FullyLoad();

    // 애니메이션 시퀀스 에셋 생성
    UAnimSequence* NewAnimSequence = NewObject<UAnimSequence>(Package, *AssetName, RF_Public | RF_Standalone);
    if (NewAnimSequence)
    {
        // 애니메이션 데이터 설정 등 추가 작업 수행
        // NewAnimSequence->Set... 등을 사용하여 애니메이션 데이터를 설정합니다.

        // 에셋 Import 데이터 설정
        UAssetImportData* ImportData = NewObject<UAssetImportData>(NewAnimSequence, UAssetImportData::StaticClass(), NAME_None, RF_Public);
        //ImportData->Update(CurrentTime);
        NewAnimSequence->AssetImportData = ImportData;

        auto skeleton = NewAnimEditor.Pin()->GetAnimationAsset()->GetSkeleton();
        NewAnimSequence->SetSkeleton(skeleton);

        // 에셋을 저장할 경로 생성
        FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageFullName, FPackageName::GetAssetPackageExtension());

        // 에셋 저장
        FAssetRegistryModule::AssetCreated(NewAnimSequence);
        Package->MarkPackageDirty();
        UPackage::SavePackage(Package, NewAnimSequence, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName);
    }
    */
}

void FMorphViewerPluginActions::FindViewportOverlay()
{
	// FSlateApplication을 통해 Active Top Level Window를 가져옵니다.
	FSlateApplication& SlateApp = FSlateApplication::Get();
	TSharedPtr<SWindow> ActiveTopLevelWindow = SlateApp.GetActiveTopLevelWindow();
    
	//SStandaloneAssetEditorToolkitHost
	TSharedPtr<SWidget> Children = ActiveTopLevelWindow->GetAllChildren()->GetChildAt(0)->GetChildren()->GetChildAt(3)->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(1)->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(0);
    StandaloneAssetEditorToolkitHost = StaticCastSharedPtr<SStandaloneAssetEditorToolkitHost>(Children);

    //DockingAreaOverlay
    TSharedPtr<SWidget> Children2 = Children->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(1)->GetChildren()->GetChildAt(0);
    DockingAreaOverlay = StaticCastSharedPtr<SOverlay>(Children2);

	//SVerticalBox
	TSharedPtr<SWidget> Children3 = Children2->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(1)->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(1)->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(0);

	//SAnimationEditorViewportTabBody
    TSharedPtr<SWidget> Children4 = Children3->GetChildren()->GetChildAt(1)->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(0);
    AnimationEditorViewportTabBody = StaticCastSharedPtr<SAnimationEditorViewportTabBody>(Children4);

	//ViewportOverlay
    TSharedPtr<SWidget> Children5 = Children4->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(0)->GetChildren()->GetChildAt(0);
    ViewportOverlay = StaticCastSharedPtr<SOverlay>(Children5);
}

TSharedRef<SDockTab> funclass::OnSpawnMyTab()
{
    //DockingAreaCanvas생성
    TSharedPtr<SCanvas> temp_canvas;
    FMorphViewerPluginActions::DockingAreaOverlay.Pin()->AddSlot()
        .VAlign(VAlign_Top)
        .HAlign(HAlign_Left)
        [
            SAssignNew(temp_canvas, SCanvas)
            .Visibility(EVisibility::Visible)
        ];

    temp_canvas->AddSlot()
        .VAlign(VAlign_Top)
        .HAlign(HAlign_Left)
        .Size(FVector2D(1920*3, 1080*3))
        [
            SAssignNew(funclass::GetInstance()->DockingAreaCanvas, SDockingAreaCanvas)
            .Visibility(EVisibility::SelfHitTestInvisible)
        ];
    UE_LOG(LogTemp, Error, TEXT("DockingAreaCanvas : %d"), funclass::GetInstance()->DockingAreaCanvas->GetId());

    //새 탭의 컨텐츠 생성
    TSharedRef<SCanvas> canvas = SNew(SCanvas).Visibility(EVisibility::Visible);
    TSharedPtr<SScrollBox> ScrollBox;

    canvas->AddSlot()
        .Size(FVector2D(300, 500))
        [
            SAssignNew(ScrollBox, SScrollBox)
            + SScrollBox::Slot()
                .VAlign(VAlign_Top)
                .HAlign(HAlign_Left)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                        .FillWidth(250.f)
                        .Padding(5.f, 5.f, 15.f, 0.f)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(TEXT("[MorphTargetList]\n")))
                        ]
                    + SHorizontalBox::Slot()
                        .AutoWidth()
                        [
                            SNew(SMyButton)
                            .Mode(FString("Reset"))
                        ]
                ]
        ];

    PreviewScene = FMorphViewerPluginActions::AnimationEditorViewportTabBody.Pin()->GetPreviewScene();
    TArray<UMorphTarget*>& MorphTargets = PreviewScene->GetPreviewMesh()->GetMorphTargets();

    for (int32 I = 0; I < MorphTargets.Num(); ++I)
    {
        UE_LOG(MorphViewerPlugin, Log, TEXT("Morph Target Name : %s"), *(MorphTargets[I]->GetName()));
        TSharedPtr<SDragAndDropVerticalBox> DragBox;

        TSharedPtr<SMySpinBox> spinbox;
        ScrollBox->AddSlot()
            [
                SAssignNew(DragBox, SDragAndDropVerticalBox)
                .Visibility(EVisibility::Visible)
                .RenderTransformPivot(FVector2D(.5, .5))
                .OnDragDetected(GetInstance().Get(), &funclass::MyOnDragDetected_Tab)
            ];
        DragBox->AddSlot()
            [
                SNew(STextBlock)
                .Text(FText::FromString(MorphTargets[I]->GetName()))
                .ColorAndOpacity(FLinearColor(1, 1, 1, 1))
            ];
        DragBox->AddSlot()
            [
                SAssignNew(spinbox, SMySpinBox)
                .MorphTargetName(MorphTargets[I]->GetName())
            ];

        funclass::GetInstance()->SpinboxList.Add(spinbox);
    }

    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        .Label(FText::FromString(TEXT("모프타깃 리스트")))
        .ContentPadding(5.0f)
        [
            canvas
        ];
}

FReply funclass::MyOnDragDetected_Viewport_Title(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, int32 i, SVerticalBox::FSlot* slot)
{
    //ViewportCanvas의 드레그 감지 ON.
    funclass::GetInstance()->ViewportCanvas->SetVisibility(EVisibility::Visible);

    bDraggedWidget_is_Title = true;

    DraggedWidgetRef_in_Viewport = MorphLayoutCanvas;

    ClickedMouseVec = MorphLayoutCanvas->GetTickSpaceGeometry().AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

    auto Operation = MakeShared<FDragAndDropVerticalBoxOp>(*(new FDragAndDropVerticalBoxOp()));
    return FReply::Handled().BeginDragDrop(Operation);
}

FReply funclass::MyOnDragDetected_Viewport(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, int32 i, SVerticalBox::FSlot* slot)
{
    //ViewportCanvas의 드레그 감지 ON.
    funclass::GetInstance()->ViewportCanvas->SetVisibility(EVisibility::Visible);

    bDraggedWidget_is_Title = false;

    //slot : STextBlock, slot's Parent : SDragAndDropVerticalBox
    DraggedWidgetRef_in_Viewport = (slot->GetWidget()->GetParentWidget());

    ClickedMouseVec = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

    auto Operation = MakeShared<FDragAndDropVerticalBoxOp>(*(new FDragAndDropVerticalBoxOp()));
    return FReply::Handled().BeginDragDrop(Operation);
}

TOptional<SDragAndDropVerticalBox::EItemDropZone> funclass::MyOnCanAcceptDrop_Viewport_Title(const FDragDropEvent& DragDropEvent, SDragAndDropVerticalBox::EItemDropZone zone, SVerticalBox::FSlot* slot)
{
    if (bDraggedWidget_is_Title)
    {
        FVector2D vec = ViewportCanvas->GetTickSpaceGeometry().AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
        MorphLayoutCanvas->SetRenderTransform(FSlateRenderTransform((vec - ClickedMouseVec)));
    }
    return SDragAndDropVerticalBox::EItemDropZone::AboveItem;
}

TOptional<SDragAndDropVerticalBox::EItemDropZone> funclass::MyOnCanAcceptDrop_Viewport(const FDragDropEvent& DragDropEvent, SDragAndDropVerticalBox::EItemDropZone zone, SVerticalBox::FSlot* slot)
{
    if (!bDraggedWidget_is_Title)
    {
        FVector2D vec = MorphLayoutCanvas->GetTickSpaceGeometry().AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
        DraggedWidgetRef_in_Viewport->SetRenderTransform(FSlateRenderTransform((vec - ClickedMouseVec)));
    }
    return SDragAndDropVerticalBox::EItemDropZone::AboveItem;
}

FReply funclass::MyOnAcceptDrop_Viewport(const FDragDropEvent& DragDropEvent, SDragAndDropVerticalBox::EItemDropZone zone, int32 i, SVerticalBox::FSlot* slot)
{
    DraggedWidgetRef_in_Viewport = nullptr;

    //ViewportCanvas의 드레그 감지 OFF.
    funclass::GetInstance()->ViewportCanvas->SetVisibility(EVisibility::SelfHitTestInvisible);

    return FReply::Handled().EndDragDrop();
}

void funclass::MyOnDragLeave_Viewport(FDragDropEvent const& DragDropEvent)
{
    //함수가 호출 될 때, 비정상적 호출인지 판별.
    auto size = FMorphViewerPluginActions::ViewportOverlay.Pin()->GetParentWidget()->GetTickSpaceGeometry().GetLocalSize();
    auto position = ViewportCanvas->GetTickSpaceGeometry().AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());

    //뷰포트 오버레이의 밖에서 호출된다면
    if (position.X <= 0 || position.Y <= 0 ||
        position.X >= size.X || position.Y >= size.Y)
    {
        //ViewportCanvas의 드레그 감지 OFF.
        funclass::GetInstance()->ViewportCanvas->SetVisibility(EVisibility::SelfHitTestInvisible);

        DraggedWidgetRef_in_Viewport = nullptr;
    }
}

FReply funclass::MyOnDragDetected_Tab(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, int32 i, SVerticalBox::FSlot* slot)
{
    //모프타깃 이름 저장
    auto temp = slot->GetWidget()->GetParentWidget()->GetChildren()->GetChildAt(1);
    ChooseSpinbox = StaticCastSharedRef<SMySpinBox>(temp);

    //DockingAreaCanvas의 드레그 감지 ON.
    DockingAreaCanvas->SetVisibility(EVisibility::Visible);
    FVector2D vec = DockingAreaCanvas->GetTickSpaceGeometry().AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    DockingAreaCanvas->AddSlot()
        .Size(CANVAS_SLOT_SIZE)
        [
            SAssignNew(DraggedWidgetShadow_Tab, SDockingAreaCanvas)
            .RenderTransformPivot(FVector2D(.5, .5))
            .RenderTransform(vec - CANVAS_SLOT_SIZE / 2)
            .Visibility(EVisibility::Visible)
        ];
    DraggedWidgetShadow_Tab->AddSlot()
        .Size(CANVAS_SLOT_SIZE)
        [
            SNew(SImage)
            .ColorAndOpacity(FLinearColor(1, 1, 0, 0.5))
		];

    auto Operation = MakeShared<FDragAndDropVerticalBoxOp>(*(new FDragAndDropVerticalBoxOp()));
    return FReply::Handled().BeginDragDrop(Operation);
}

void funclass::AddCurveData()
{
    //Curve 데이터 추가.
	//FAnimTimelineTrack_Curves::FillVariableCurveMenu(...) 참조
	//FAnimTimelineTrack_Curves::AddVariableCurve(...) 참조
    TSharedPtr<MyAnimationEditor> AnimEditor = FMorphViewerPluginActions::NewAnimEditor.Pin();
    UAnimSequenceBase* AnimSequenceBase = Cast<UAnimSequenceBase>(AnimEditor->GetSequenceEditor()->GetEditorObject());
    USkeleton* CurrentSkeleton = AnimSequenceBase->GetSkeleton();
    check(CurrentSkeleton);

    const FSmartNameMapping* Mapping = CurrentSkeleton->GetSmartNameContainer(USkeleton::AnimCurveMappingName);
    TArray<USkeleton::AnimCurveUID> CurveUids;
    Mapping->FillUidArray(CurveUids);
    TArray<FSmartNameSortItem> SmartNameList;

    for (USkeleton::AnimCurveUID Id : CurveUids)
    {
        if (!AnimSequenceBase->RawCurveData.GetCurveData(Id))
        {
            FName CurveName;
            if (Mapping->GetName(Id, CurveName))
            {
                SmartNameList.Add(FSmartNameSortItem(CurveName, Id));
            }
        }
    }
    SmartNameList.Sort(FSmartNameSortItemSortOp());

    FString ChooseName = funclass::GetInstance()->ChooseSpinbox->MorphTargetName;
    for (FSmartNameSortItem SmartNameItem : SmartNameList)
    {
        if (ChooseName == SmartNameItem.SmartName.ToString())
        {
            AnimSequenceBase->Modify();

            FSmartName NewName;
            ensureAlways(CurrentSkeleton->GetSmartNameByUID(USkeleton::AnimCurveMappingName, SmartNameItem.ID, NewName));
            AnimSequenceBase->RawCurveData.AddCurveData(NewName);
            AnimSequenceBase->MarkRawDataAsModified();

            //새로운 Curve를 추가했으므로, AnimSequenceBase의 CurveData를 다시 불러온다.
            AnimEditor->ReOpenAnimationDocumentTab();
        }
    }
}

TSharedPtr<funclass> funclass::GetInstance()
{
    if (!Instance.IsValid())
    {
        Instance = MakeShareable<funclass>(new funclass());
        return Instance;
    }
    else
        return Instance;
}