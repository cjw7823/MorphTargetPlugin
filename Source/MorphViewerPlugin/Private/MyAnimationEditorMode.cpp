// Fill out your copyright notice in the Description page of Project Settings.


#include "MyAnimationEditorMode.h"
#include "Modules/ModuleManager.h"
#include "PersonaModule.h"
#include "MyAnimationEditor.h"
#include "ISkeletonTree.h"
#include "ISkeletonEditorModule.h"
#include "IPersonaToolkit.h"

MyAnimationEditorMode::MyAnimationEditorMode(TSharedRef<FWorkflowCentricApplication> InHostingApp, TSharedRef<ISkeletonTree> InSkeletonTree)
	: FApplicationMode(AnimationEditorModes::AnimationEditorMode)
{
	HostingAppPtr = InHostingApp;

	TSharedRef<MyAnimationEditor> AnimationEditor = StaticCastSharedRef<MyAnimationEditor>(InHostingApp);

	ISkeletonEditorModule& SkeletonEditorModule = FModuleManager::LoadModuleChecked<ISkeletonEditorModule>("SkeletonEditor");
	TabFactories.RegisterFactory(SkeletonEditorModule.CreateSkeletonTreeTabFactory(InHostingApp, InSkeletonTree));

	FOnObjectsSelected OnObjectsSelected = FOnObjectsSelected::CreateSP(&AnimationEditor.Get(), &MyAnimationEditor::HandleObjectsSelected);

	FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>("Persona");
	TabFactories.RegisterFactory(PersonaModule.CreateDetailsTabFactory(InHostingApp, FOnDetailsCreated::CreateSP(&AnimationEditor.Get(), &MyAnimationEditor::HandleDetailsCreated)));

	FPersonaViewportArgs ViewportArgs(AnimationEditor->GetPersonaToolkit()->GetPreviewScene());
	ViewportArgs.bShowTimeline = false;
	ViewportArgs.ContextName = TEXT("AnimationEditor.Viewport");

	PersonaModule.RegisterPersonaViewportTabFactories(TabFactories, InHostingApp, ViewportArgs);

	TabFactories.RegisterFactory(PersonaModule.CreateAdvancedPreviewSceneTabFactory(InHostingApp, AnimationEditor->GetPersonaToolkit()->GetPreviewScene()));
	TabFactories.RegisterFactory(PersonaModule.CreateAnimationAssetBrowserTabFactory(InHostingApp, AnimationEditor->GetPersonaToolkit(), FOnOpenNewAsset::CreateSP(&AnimationEditor.Get(), &MyAnimationEditor::HandleOpenNewAsset), FOnAnimationSequenceBrowserCreated::CreateSP(&AnimationEditor.Get(), &MyAnimationEditor::HandleAnimationSequenceBrowserCreated), true));
	TabFactories.RegisterFactory(PersonaModule.CreateAssetDetailsTabFactory(InHostingApp, FOnGetAsset::CreateSP(&AnimationEditor.Get(), &MyAnimationEditor::HandleGetAsset), FOnDetailsCreated()));
	TabFactories.RegisterFactory(PersonaModule.CreateCurveViewerTabFactory(InHostingApp, InSkeletonTree->GetEditableSkeleton(), AnimationEditor->GetPersonaToolkit()->GetPreviewScene(), AnimationEditor->OnPostUndo, OnObjectsSelected));
	TabFactories.RegisterFactory(PersonaModule.CreateSkeletonSlotNamesTabFactory(InHostingApp, InSkeletonTree->GetEditableSkeleton(), AnimationEditor->OnPostUndo, FOnObjectSelected::CreateSP(&AnimationEditor.Get(), &MyAnimationEditor::HandleObjectSelected)));
	TabFactories.RegisterFactory(PersonaModule.CreateAnimNotifiesTabFactory(InHostingApp, InSkeletonTree->GetEditableSkeleton(), OnObjectsSelected));
	TabFactories.RegisterFactory(PersonaModule.CreateAnimMontageSectionsTabFactory(InHostingApp, AnimationEditor->GetPersonaToolkit(), AnimationEditor->OnSectionsChanged));

	TabLayout = FTabManager::NewLayout("Standalone_AnimationEditor_Layout_v1.3")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->SetHideTabWell(true)
				->AddTab(InHostingApp->GetToolbarTabId(), ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewSplitter()
				->SetSizeCoefficient(0.9f)
				->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient(0.2f)
					->SetOrientation(Orient_Vertical)
					->Split
					(
						FTabManager::NewStack()
						->SetHideTabWell(false)
						->AddTab(AnimationEditorTabs::SkeletonTreeTab, ETabState::OpenedTab)
						->AddTab(AnimationEditorTabs::AssetDetailsTab, ETabState::OpenedTab)
					)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient(0.6f)
					->SetOrientation(Orient_Vertical)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.6f)
						->SetHideTabWell(true)
						->AddTab(AnimationEditorTabs::ViewportTab, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.4f)
						->SetHideTabWell(true)
						->AddTab(AnimationEditorTabs::DocumentTab, ETabState::ClosedTab)
						->AddTab(AnimationEditorTabs::CurveEditorTab, ETabState::ClosedTab)
						->SetForegroundTab(AnimationEditorTabs::DocumentTab)
					)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient(0.2f)
					->SetOrientation(Orient_Vertical)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.6f)
						->SetHideTabWell(false)
						->AddTab(AnimationEditorTabs::DetailsTab, ETabState::OpenedTab)
						->AddTab(AnimationEditorTabs::AdvancedPreviewTab, ETabState::OpenedTab)
						->SetForegroundTab(AnimationEditorTabs::DetailsTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.4f)
						->SetHideTabWell(false)
						->AddTab(AnimationEditorTabs::AssetBrowserTab, ETabState::OpenedTab)
						->AddTab(AnimationEditorTabs::AnimMontageSectionsTab, ETabState::ClosedTab)
						->AddTab(AnimationEditorTabs::CurveNamesTab, ETabState::ClosedTab)
						->AddTab(AnimationEditorTabs::SlotNamesTab, ETabState::ClosedTab)
						->AddTab(AnimationEditorTabs::MorphTargetListTab, ETabState::ClosedTab)
					)
				)
			)
		);

	PersonaModule.OnRegisterTabs().Broadcast(TabFactories, InHostingApp);
	LayoutExtender = MakeShared<FLayoutExtender>();
	PersonaModule.OnRegisterLayoutExtensions().Broadcast(*LayoutExtender.Get());
	TabLayout->ProcessExtensions(*LayoutExtender.Get());
}

void MyAnimationEditorMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	TSharedPtr<FWorkflowCentricApplication> HostingApp = HostingAppPtr.Pin();
	HostingApp->RegisterTabSpawners(InTabManager.ToSharedRef());
	HostingApp->PushTabFactories(TabFactories);

	FApplicationMode::RegisterTabFactories(InTabManager);
}

void MyAnimationEditorMode::AddTabFactory(FCreateWorkflowTabFactory FactoryCreator)
{
	if (FactoryCreator.IsBound())
	{
		TabFactories.RegisterFactory(FactoryCreator.Execute(HostingAppPtr.Pin()));
	}
}

void MyAnimationEditorMode::RemoveTabFactory(FName TabFactoryID)
{
	TabFactories.UnregisterFactory(TabFactoryID);
}
