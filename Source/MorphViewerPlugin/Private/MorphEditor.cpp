// Fill out your copyright notice in the Description page of Project Settings.


#include "MorphEditor.h"
#include "MorphViewerPluginStyle.h"
#include "Morph_Viewport.h"

#include "PropertyEditorModule.h"

#include "Persona/Private/AnimationEditorPreviewScene.h"
#include "AnimationEditor/Private/AnimationEditor.h"
#include "PersonaModule.h"
#include "IPersonaToolkit.h"
#include "IPersonaPreviewScene.h"
#include "IAssetFamily.h"
#include "ISkeletonTree.h"
#include "ISkeletonTreeItem.h"
#include "ISkeletonEditorModule.h"
#include "Algo/Transform.h"

#include "AdvancedPreviewSceneModule.h"

#include "Persona/Private/SAnimSequenceCurveEditor.h"

const FName FMorphEditor::MorphEditorAppIdentifier = FName(TEXT("MorphEditorApp"));
const FName FMorphEditor::ViewportTabId = FName(TEXT("Morph Viewport"));
const FName FMorphEditor::DetailTabId = FName(TEXT("Morph Detail"));
const FName FMorphEditor::PreviewSceneSettingsTabId = FName(TEXT("Morph PreviewScene Setting"));
const FName FMorphEditor::MorphSkeletonTreeId = FName(TEXT("MorphSkeletonTree"));

#define LOCTEXT_NAMESPACE "MorphEditor"

FMorphEditor::~FMorphEditor()
{
	DetailsView.Reset();
	AdvancedPreviewSettingsWidget.Reset();
	SkeletonTree.Reset();
}

void FMorphEditor::InitMorphEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UAnimationAsset* InAnimationAsset)
{
	FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>("Persona");
	PersonaToolkit = PersonaModule.CreatePersonaToolkit(InAnimationAsset);
	PersonaToolkit->GetPreviewScene()->SetDefaultAnimationMode(EPreviewSceneDefaultAnimationMode::Animation);

	//스켈레톤트리 생성
	FSkeletonTreeArgs SkeletonTreeArgs;
	SkeletonTreeArgs.OnSelectionChanged = FOnSkeletonTreeSelectionChanged::CreateSP(this, &FMorphEditor::HandleSelectionChanged);
	SkeletonTreeArgs.PreviewScene = PersonaToolkit->GetPreviewScene();
	SkeletonTreeArgs.ContextName = GetToolkitFName();
	ISkeletonEditorModule& SkeletonEditorModule = FModuleManager::GetModuleChecked<ISkeletonEditorModule>("SkeletonEditor");
	SkeletonTree = SkeletonEditorModule.CreateSkeletonTree(PersonaToolkit->GetSkeleton(), SkeletonTreeArgs);

	// 편집하기 위해 들어온 IGC 객체의 설정.
	InAnimationAsset->SetFlags(RF_Transactional); // Undo, Redo의 지원.
	MorphObject = InAnimationAsset;

	// 프로퍼티에디터 모듈을 가져와서 디테일 뷰를 생성.
	const bool bIsUpdatable = false;
	const bool bAllowFavorites = true;
	const bool bIsLockable = false;
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	const FDetailsViewArgs DetailsViewArgs(bIsUpdatable, bIsLockable, true, FDetailsViewArgs::ObjectsUseNameArea, false);
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	// 뷰포트 생성.
	Viewport = SNew(SMorph_Viewport)
		.ParentMorphEditor(SharedThis(this))
		.ObjectToEdit(MorphObject)
		.PersonaToolkit_(PersonaToolkit);

	//테스트 위젯 생성.
	//panel = SNew(SAnimSequenceCurveEditor, PersonaToolkit->GetPreviewScene(), MorphObject);

	// 프리뷰 씬 세팅 위젯.
	FAdvancedPreviewSceneModule& AdvancedPreviewSceneModule = FModuleManager::LoadModuleChecked<FAdvancedPreviewSceneModule>("AdvancedPreviewScene");

	AdvancedPreviewSettingsWidget = AdvancedPreviewSceneModule.CreateAdvancedPreviewSceneSettingsWidget(StaticCastSharedRef<FAdvancedPreviewScene, FAnimationEditorPreviewScene>(Viewport->GetPreviewScene()));

	// 툴바가 들어갈 기본 레이아웃 설계.
	const TSharedRef<FTabManager::FLayout> EditorDefaultLayout = FTabManager::NewLayout("MorphEditor_vvvv")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->AddTab(FAssetEditorToolkit::GetToolbarTabId(), ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.3)
					->AddTab(MorphSkeletonTreeId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.5)
					->AddTab(ViewportTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.2f)
						->AddTab(DetailTabId, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
						->Split
						(
							FTabManager::NewStack()
							->AddTab(PreviewSceneSettingsTabId, ETabState::OpenedTab)
						)
					)
				)
			)
		);

	// 에디터 초기화.
	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;

	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, MorphEditorAppIdentifier, EditorDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, InAnimationAsset);

	// 디테일 뷰에 편집 객체를 지정.
	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(MorphObject);
	}
}

void FMorphEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& tabManager)
{
	FAssetEditorToolkit::WorkspaceMenuCategory = tabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_MorphAssetEditor", "Morph Asset Editor"));
	auto WorkspaceMenuCategoryRef = FAssetEditorToolkit::WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(tabManager);

	tabManager->RegisterTabSpawner(ViewportTabId, FOnSpawnTab::CreateSP(this, &FMorphEditor::SpawnTab_Viewport))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FMorphViewerPluginStyle::GetStyleSetName(), "IGCExtensions.Command1"));

	tabManager->RegisterTabSpawner(DetailTabId, FOnSpawnTab::CreateSP(this, &FMorphEditor::SpawnTab_Detail))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FMorphViewerPluginStyle::GetStyleSetName(), "IGCExtensions.Command2"));

	tabManager->RegisterTabSpawner(PreviewSceneSettingsTabId, FOnSpawnTab::CreateSP(this, &FMorphEditor::SpawnTab_PreviewSceneSettings))
		.SetDisplayName(LOCTEXT("PreviewSceneTab", "Preview Scene Settings"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FMorphViewerPluginStyle::GetStyleSetName(), "IGCExtensions.Command3"));

	tabManager->RegisterTabSpawner(MorphSkeletonTreeId, FOnSpawnTab::CreateSP(this, &FMorphEditor::SpawnTab_SkeletonTree))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FMorphViewerPluginStyle::GetStyleSetName(), "IGCExtensions.Command3"));
}

void FMorphEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& tabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(tabManager);

	tabManager->UnregisterTabSpawner(ViewportTabId);
	tabManager->UnregisterTabSpawner(DetailTabId);
	tabManager->UnregisterTabSpawner(PreviewSceneSettingsTabId);
	tabManager->UnregisterTabSpawner(MorphSkeletonTreeId);
}

FName FMorphEditor::GetToolkitFName() const
{
	return FName("Morph Editor");
}

FText FMorphEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Morph Editor");
}

FString FMorphEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Morph ").ToString();
}

FLinearColor FMorphEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.0f, 0.0f, 0.2f, 0.5f);
}

void FMorphEditor::HandleSelectionChanged(const TArrayView<TSharedPtr<class ISkeletonTreeItem>>& InSelectedItems, ESelectInfo::Type InSelectInfo)
{
	if (DetailsView.IsValid())
	{
		TArray<UObject*> Objects;
		Algo::TransformIf(InSelectedItems, Objects, [](const TSharedPtr<ISkeletonTreeItem>& InItem) { return InItem->GetObject() != nullptr; }, [](const TSharedPtr<ISkeletonTreeItem>& InItem) { return InItem->GetObject(); });
		DetailsView->SetObjects(Objects);
	}
}

TSharedRef<SDockTab> FMorphEditor::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == ViewportTabId);
	return SNew(SDockTab)
		[
			Viewport.ToSharedRef()
		];
}

TSharedRef<SDockTab> FMorphEditor::SpawnTab_Detail(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == DetailTabId);
	return SNew(SDockTab)
		[
			DetailsView.ToSharedRef()
		];
}

TSharedRef<SDockTab> FMorphEditor::SpawnTab_PreviewSceneSettings(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == PreviewSceneSettingsTabId);
	return SNew(SDockTab)
		[
			AdvancedPreviewSettingsWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FMorphEditor::SpawnTab_SkeletonTree(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == MorphSkeletonTreeId);
	return SNew(SDockTab)
		[
			SkeletonTree.ToSharedRef()
		];
}

#undef LOCTEXT_NAMESPACE