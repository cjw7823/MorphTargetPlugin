// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyAnimationEditor.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"
#include "EditorReimportHandler.h"
#include "Animation/SmartName.h"
#include "Animation/AnimationAsset.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "AssetData.h"
#include "EdGraph/EdGraphSchema.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Editor/EditorEngine.h"
#include "Factories/AnimSequenceFactory.h"
#include "Factories/PoseAssetFactory.h"
#include "EngineGlobals.h"
#include "Editor.h"
#include "IAnimationEditorModule.h"
#include "IPersonaToolkit.h"
#include "PersonaModule.h"
#include "MyAnimationEditorMode.h"
#include "IPersonaPreviewScene.h"
#include "AnimationEditor/Private/AnimationEditorCommands.h"
#include "AnimationEditor/Private/AnimationEditorCommands.cpp"
#include "IDetailsView.h"
#include "ISkeletonTree.h"
#include "ISkeletonEditorModule.h"
#include "IDocumentation.h"
#include "Widgets/Docking/SDockTab.h"
#include "Animation/PoseAsset.h"
#include "AnimGraph/Classes/AnimPreviewInstance.h"
#include "ScopedTransaction.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "AnimationEditorUtils.h"
#include "AssetRegistryModule.h"
#include "IAssetFamily.h"
#include "IAnimationSequenceBrowser.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "PersonaCommonCommands.h"
#include "Sound/SoundWave.h"
#include "Engine/CurveTable.h"
#include "Developer/AssetTools/Public/IAssetTools.h"
#include "Developer/AssetTools/Public/AssetToolsModule.h"
#include "ISkeletonTreeItem.h"
#include "Algo/Transform.h"
#include "SequenceRecorder/Public/ISequenceRecorder.h"
#include "IAnimSequenceCurveEditor.h"

#include "MyAnimSequenceCurveEditor.h"
#include "Persona/Private/SSequenceEditor.h"
#include "MorphViewerPluginCommands.h"

const FName MyAnimationEditorAppIdentifier = FName(TEXT("MyAnimationEditorApp"));

const FName AnimationEditorModes::AnimationEditorMode(TEXT("AnimationEditorMode"));

const FName AnimationEditorTabs::DetailsTab(TEXT("DetailsTab"));
const FName AnimationEditorTabs::SkeletonTreeTab(TEXT("SkeletonTreeView"));
const FName AnimationEditorTabs::ViewportTab(TEXT("Viewport"));
const FName AnimationEditorTabs::AdvancedPreviewTab(TEXT("AdvancedPreviewTab"));
const FName AnimationEditorTabs::DocumentTab(TEXT("Document"));
const FName AnimationEditorTabs::CurveEditorTab(TEXT("CurveEditor"));
const FName AnimationEditorTabs::AssetBrowserTab(TEXT("SequenceBrowser"));
const FName AnimationEditorTabs::AssetDetailsTab(TEXT("AnimAssetPropertiesTab"));
const FName AnimationEditorTabs::CurveNamesTab(TEXT("AnimCurveViewerTab"));
const FName AnimationEditorTabs::SlotNamesTab(TEXT("SkeletonSlotNames"));
const FName AnimationEditorTabs::AnimMontageSectionsTab(TEXT("AnimMontageSections"));
const FName AnimationEditorTabs::MorphTargetListTab(TEXT("MorphTargetListTab"));

DEFINE_LOG_CATEGORY(LogAnimationEditor);

#define LOCTEXT_NAMESPACE "AnimationEditor"

int32 MyAnimationEditor::Count = 0;

MyAnimationEditor::MyAnimationEditor()
{
	Count++;
	UE_LOG(LogAnimationEditor, Log, TEXT("MyAnimationEditor::Count: %d"), Count);

	UEditorEngine* Editor = Cast<UEditorEngine>(GEngine);
	if (Editor != nullptr)
	{
		Editor->RegisterForUndo(this);
	}
}

MyAnimationEditor::~MyAnimationEditor()
{
	Count--;
	UE_LOG(LogAnimationEditor, Log, TEXT("MyAnimationEditor::Count: %d"), Count);
	funclass::GetInstance()->ViewportSpinboxList.Empty();

	UEditorEngine* Editor = Cast<UEditorEngine>(GEngine);
	if (Editor != nullptr)
	{
		Editor->UnregisterForUndo(this);
	}

	GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetPostImport.RemoveAll(this);
	FReimportManager::Instance()->OnPostReimport().RemoveAll(this);
}

void MyAnimationEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_AnimationEditor", "Animation Editor"));

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}

void MyAnimationEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
}

bool MyAnimationEditor::ReOpenAnimationDocumentTab()
{
	if (AnimationAsset)
	{
		OpenNewAnimationDocumentTab(AnimationAsset);
		return true;
	}
	return false;
}

TSharedPtr<SSequenceEditor> MyAnimationEditor::GetSequenceEditor()
{
	return SequenceEditor;
}

void MyAnimationEditor::InitAnimationEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UAnimationAsset* InAnimationAsset)
{
	AnimationAsset = InAnimationAsset;

	// Register post import callback to catch animation imports when we have the asset open (we need to reinit)
	FReimportManager::Instance()->OnPostReimport().AddRaw(this, &MyAnimationEditor::HandlePostReimport);
	GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetPostImport.AddRaw(this, &MyAnimationEditor::HandlePostImport);

	FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>("Persona");
	PersonaToolkit = PersonaModule.CreatePersonaToolkit(InAnimationAsset);

	PersonaToolkit->GetPreviewScene()->SetDefaultAnimationMode(EPreviewSceneDefaultAnimationMode::Animation);

	FSkeletonTreeArgs SkeletonTreeArgs;
	SkeletonTreeArgs.OnSelectionChanged = FOnSkeletonTreeSelectionChanged::CreateSP(this, &MyAnimationEditor::HandleSelectionChanged);
	SkeletonTreeArgs.PreviewScene = PersonaToolkit->GetPreviewScene();
	SkeletonTreeArgs.ContextName = GetToolkitFName();

	ISkeletonEditorModule& SkeletonEditorModule = FModuleManager::GetModuleChecked<ISkeletonEditorModule>("SkeletonEditor");
	SkeletonTree = SkeletonEditorModule.CreateSkeletonTree(PersonaToolkit->GetSkeleton(), SkeletonTreeArgs);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, MyAnimationEditorAppIdentifier, FTabManager::FLayout::NullLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, InAnimationAsset);

	BindCommands();

	AddApplicationMode(
		AnimationEditorModes::AnimationEditorMode,
		MakeShareable(new MyAnimationEditorMode(SharedThis(this), SkeletonTree.ToSharedRef())));

	SetCurrentMode(AnimationEditorModes::AnimationEditorMode);

	ExtendMenu();
	ExtendToolbar();
	RegenerateMenusAndToolbars();

	OpenNewAnimationDocumentTab(AnimationAsset);
}

FName MyAnimationEditor::GetToolkitFName() const
{
	return FName("AnimationEditor");
}

FText MyAnimationEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "AnimationEditor");
}

FString MyAnimationEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "AnimationEditor ").ToString();
}

FLinearColor MyAnimationEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f);
}

void MyAnimationEditor::Tick(float DeltaTime)
{
	GetPersonaToolkit()->GetPreviewScene()->InvalidateViews();
}

TStatId MyAnimationEditor::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FAnimationEditor, STATGROUP_Tickables);
}

void MyAnimationEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(AnimationAsset);
}

void MyAnimationEditor::BindCommands()
{
	FAnimationEditorCommands::Register();

	ToolkitCommands->MapAction(FAnimationEditorCommands::Get().ApplyCompression,
		FExecuteAction::CreateSP(this, &MyAnimationEditor::OnApplyCompression),
		FCanExecuteAction::CreateSP(this, &MyAnimationEditor::HasValidAnimationSequence));

	ToolkitCommands->MapAction(FAnimationEditorCommands::Get().SetKey,
		FExecuteAction::CreateSP(this, &MyAnimationEditor::OnSetKey),
		FCanExecuteAction::CreateSP(this, &MyAnimationEditor::CanSetKey));

	ToolkitCommands->MapAction(FAnimationEditorCommands::Get().ReimportAnimation,
		FExecuteAction::CreateSP(this, &MyAnimationEditor::OnReimportAnimation),
		FCanExecuteAction::CreateSP(this, &MyAnimationEditor::HasValidAnimationSequence));

	ToolkitCommands->MapAction(FAnimationEditorCommands::Get().ApplyAnimation,
		FExecuteAction::CreateSP(this, &MyAnimationEditor::OnApplyRawAnimChanges),
		FCanExecuteAction::CreateSP(this, &MyAnimationEditor::CanApplyRawAnimChanges));

	ToolkitCommands->MapAction(FAnimationEditorCommands::Get().ExportToFBX_AnimData,
		FExecuteAction::CreateSP(this, &MyAnimationEditor::OnExportToFBX, EExportSourceOption::CurrentAnimation_AnimData),
		FCanExecuteAction::CreateSP(this, &MyAnimationEditor::HasValidAnimationSequence));

	ToolkitCommands->MapAction(FAnimationEditorCommands::Get().ExportToFBX_PreviewMesh,
		FExecuteAction::CreateSP(this, &MyAnimationEditor::OnExportToFBX, EExportSourceOption::CurrentAnimation_PreviewMesh),
		FCanExecuteAction::CreateSP(this, &MyAnimationEditor::HasValidAnimationSequence));

	ToolkitCommands->MapAction(FAnimationEditorCommands::Get().AddLoopingInterpolation,
		FExecuteAction::CreateSP(this, &MyAnimationEditor::OnAddLoopingInterpolation),
		FCanExecuteAction::CreateSP(this, &MyAnimationEditor::HasValidAnimationSequence));

	ToolkitCommands->MapAction(FAnimationEditorCommands::Get().RemoveBoneTracks,
		FExecuteAction::CreateSP(this, &MyAnimationEditor::OnRemoveBoneTrack),
		FCanExecuteAction::CreateSP(this, &MyAnimationEditor::HasValidAnimationSequence));

	ToolkitCommands->MapAction(FPersonaCommonCommands::Get().TogglePlay,
		FExecuteAction::CreateRaw(&GetPersonaToolkit()->GetPreviewScene().Get(), &IPersonaPreviewScene::TogglePlayback));
}

void MyAnimationEditor::ExtendToolbar()
{
	// If the ToolbarExtender is valid, remove it before rebuilding it
	if (ToolbarExtender.IsValid())
	{
		RemoveToolbarExtender(ToolbarExtender);
		ToolbarExtender.Reset();
	}

	ToolbarExtender = MakeShareable(new FExtender);

	AddToolbarExtender(ToolbarExtender);

	IAnimationEditorModule& AnimationEditorModule = FModuleManager::GetModuleChecked<IAnimationEditorModule>("AnimationEditor");
	AddToolbarExtender(AnimationEditorModule.GetToolBarExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

	TArray<IAnimationEditorModule::FAnimationEditorToolbarExtender> ToolbarExtenderDelegates = AnimationEditorModule.GetAllAnimationEditorToolbarExtenders();

	for (auto& ToolbarExtenderDelegate : ToolbarExtenderDelegates)
	{
		if (ToolbarExtenderDelegate.IsBound())
		{
			AddToolbarExtender(ToolbarExtenderDelegate.Execute(GetToolkitCommands(), SharedThis(this)));
		}
	}

	// extend extra menu/toolbars
	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& ToolbarBuilder)
			{
				FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>("Persona");
				FPersonaModule::FCommonToolbarExtensionArgs Args;
				Args.bPreviewAnimation = false;
				Args.bReferencePose = false;
				PersonaModule.AddCommonToolbarExtensions(ToolbarBuilder, PersonaToolkit.ToSharedRef(), Args);

				ToolbarBuilder.BeginSection("Animation");
				{
					ToolbarBuilder.AddToolBarButton(FAnimationEditorCommands::Get().ReimportAnimation);
					ToolbarBuilder.AddToolBarButton(FAnimationEditorCommands::Get().ApplyCompression, NAME_None, LOCTEXT("Toolbar_ApplyCompression", "Compression"));

					{
						ToolbarBuilder.AddComboButton(
							FUIAction(),
							FOnGetContent::CreateSP(this, &MyAnimationEditor::GenerateExportAssetMenu),
							LOCTEXT("ExportAsset_Label", "Export Asset"),
							LOCTEXT("ExportAsset_ToolTip", "Export Assets for this skeleton."),
							FSlateIcon(FEditorStyle::GetStyleSetName(), "Persona.ExportToFBX")
						);
					}
				}
				ToolbarBuilder.EndSection();

				ToolbarBuilder.BeginSection("Editing");
				{
					ToolbarBuilder.AddToolBarButton(FAnimationEditorCommands::Get().SetKey, NAME_None, LOCTEXT("Toolbar_SetKey", "Key"));
					ToolbarBuilder.AddToolBarButton(FAnimationEditorCommands::Get().ApplyAnimation, NAME_None, LOCTEXT("Toolbar_ApplyAnimation", "Apply"));
				}
				ToolbarBuilder.EndSection();

				TSharedRef<class IAssetFamily> AssetFamily = PersonaModule.CreatePersonaAssetFamily(AnimationAsset);
				AddToolbarWidget(PersonaModule.CreateAssetFamilyShortcutWidget(SharedThis(this), AssetFamily));
			}
	));
}

void MyAnimationEditor::ExtendMenu()
{
	MenuExtender = MakeShareable(new FExtender);

	struct Local
	{
		static void AddAssetMenu(FMenuBuilder& MenuBuilder, MyAnimationEditor* InAnimationEditor)
		{
			MenuBuilder.BeginSection("AnimationEditor", LOCTEXT("AnimationEditorAssetMenu_Animation", "Animation"));
			{
				MenuBuilder.AddMenuEntry(FAnimationEditorCommands::Get().ApplyCompression);

				MenuBuilder.AddSubMenu(
					LOCTEXT("ExportToFBX", "Export to FBX"),
					LOCTEXT("ExportToFBX_ToolTip", "Export current animation to FBX"),
					FNewMenuDelegate::CreateSP(InAnimationEditor, &MyAnimationEditor::FillExportAssetMenu),
					false,
					FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.")
				);

				MenuBuilder.AddMenuEntry(FAnimationEditorCommands::Get().AddLoopingInterpolation);
				MenuBuilder.AddMenuEntry(FAnimationEditorCommands::Get().RemoveBoneTracks);

				MenuBuilder.AddSubMenu(
					LOCTEXT("CopyCurvesToSoundWave", "Copy Curves To SoundWave"),
					LOCTEXT("CopyCurvesToSoundWave_ToolTip", "Copy curves from this animation to the selected SoundWave"),
					FNewMenuDelegate::CreateSP(InAnimationEditor, &MyAnimationEditor::FillCopyToSoundWaveMenu),
					false,
					FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.")
				);
			}
			MenuBuilder.EndSection();
		}
	};

	MenuExtender->AddMenuExtension(
		"AssetEditorActions",
		EExtensionHook::After,
		GetToolkitCommands(),
		FMenuExtensionDelegate::CreateStatic(&Local::AddAssetMenu, this)
	);

	AddMenuExtender(MenuExtender);

	IAnimationEditorModule& AnimationEditorModule = FModuleManager::GetModuleChecked<IAnimationEditorModule>("AnimationEditor");
	AddMenuExtender(AnimationEditorModule.GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
}

void MyAnimationEditor::HandleObjectsSelected(const TArray<UObject*>& InObjects)
{
	if (DetailsView.IsValid())
	{
		DetailsView->SetObjects(InObjects);
	}
}

void MyAnimationEditor::HandleSelectionChanged(const TArrayView<TSharedPtr<ISkeletonTreeItem>>& InSelectedItems, ESelectInfo::Type InSelectInfo)
{
	if (DetailsView.IsValid())
	{
		TArray<UObject*> Objects;
		Algo::TransformIf(InSelectedItems, Objects, [](const TSharedPtr<ISkeletonTreeItem>& InItem) { return InItem->GetObject() != nullptr; }, [](const TSharedPtr<ISkeletonTreeItem>& InItem) { return InItem->GetObject(); });
		DetailsView->SetObjects(Objects);
	}
}

void MyAnimationEditor::HandleObjectSelected(UObject* InObject)
{
	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(InObject);
	}
}

void MyAnimationEditor::PostUndo(bool bSuccess)
{
	OnPostUndo.Broadcast();
}

void MyAnimationEditor::PostRedo(bool bSuccess)
{
	OnPostUndo.Broadcast();
}

void MyAnimationEditor::HandleDetailsCreated(const TSharedRef<IDetailsView>& InDetailsView)
{
	DetailsView = InDetailsView;
}

TSharedPtr<SDockTab> MyAnimationEditor::OpenNewAnimationDocumentTab(UAnimationAsset* InAnimAsset)
{
	TSharedPtr<SDockTab> OpenedTab;

	if (InAnimAsset != nullptr)
	{
		FString	DocumentLink;

		FAnimDocumentArgs Args(PersonaToolkit->GetPreviewScene(), GetPersonaToolkit(), GetSkeletonTree()->GetEditableSkeleton(), OnPostUndo, OnSectionsChanged);
		Args.OnDespatchObjectsSelected = FOnObjectsSelected::CreateSP(this, &MyAnimationEditor::HandleObjectsSelected);
		Args.OnDespatchInvokeTab = FOnInvokeTab::CreateSP(this, &FAssetEditorToolkit::InvokeTab);
		Args.OnDespatchSectionsChanged = FSimpleDelegate::CreateSP(this, &MyAnimationEditor::HandleSectionsChanged);

		FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");
		TSharedPtr<SWidget>	TabContents = PersonaModule.CreateEditorWidgetForAnimDocument(SharedThis(this), InAnimAsset, Args, DocumentLink);
		SequenceEditor = StaticCastSharedPtr<SSequenceEditor>(TabContents);

		if (AnimationAsset)
		{
			RemoveEditingObject(AnimationAsset);
		}

		AddEditingObject(InAnimAsset);
		AnimationAsset = InAnimAsset;

		GetPersonaToolkit()->GetPreviewScene()->SetPreviewAnimationAsset(InAnimAsset);
		GetPersonaToolkit()->SetAnimationAsset(InAnimAsset);

		// Close existing opened curve tab
		if (AnimCurveDocumentTab.IsValid())
		{
			AnimCurveDocumentTab.Pin()->RequestCloseTab();
		}

		AnimCurveDocumentTab.Reset();

		struct Local
		{
			static FText GetObjectName(UObject* Object)
			{
				return FText::FromString(Object->GetName());
			}
		};

		TAttribute<FText> NameAttribute = TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateStatic(&Local::GetObjectName, (UObject*)InAnimAsset));

		if (SharedAnimDocumentTab.IsValid())
		{
			OpenedTab = SharedAnimDocumentTab.Pin();
			OpenedTab->SetContent(TabContents.ToSharedRef());
			OpenedTab->ActivateInParent(ETabActivationCause::SetDirectly);
			OpenedTab->SetLabel(NameAttribute);
			OpenedTab->SetLeftContent(IDocumentation::Get()->CreateAnchor(DocumentLink));
		}
		else
		{
			OpenedTab = SNew(SDockTab)
				.Label(NameAttribute)
				.TabRole(ETabRole::DocumentTab)
				.TabColorScale(GetTabColorScale())
				.OnTabClosed_Lambda([this](TSharedRef<SDockTab> InTab)
					{
						TSharedPtr<SDockTab> CurveTab = AnimCurveDocumentTab.Pin();
						if (CurveTab.IsValid())
						{
							CurveTab->RequestCloseTab();
						}
					})
				[
					TabContents.ToSharedRef()
				];

					OpenedTab->SetLeftContent(IDocumentation::Get()->CreateAnchor(DocumentLink));

					TabManager->InsertNewDocumentTab(AnimationEditorTabs::DocumentTab, FTabManager::ESearchPreference::RequireClosedTab, OpenedTab.ToSharedRef());

					SharedAnimDocumentTab = OpenedTab;
		}

		// Invoke the preview tab if this is a montage
		if (InAnimAsset->IsA<UAnimMontage>())
		{
			TabManager->TryInvokeTab(AnimationEditorTabs::AnimMontageSectionsTab);
			OnSectionsChanged.Broadcast();
		}
		else
		{
			// Close existing opened montage sections tab
			TSharedPtr<SDockTab> OpenMontageSectionsTab = TabManager->FindExistingLiveTab(AnimationEditorTabs::AnimMontageSectionsTab);
			if (OpenMontageSectionsTab.IsValid())
			{
				OpenMontageSectionsTab->RequestCloseTab();
			}
		}

		if (SequenceBrowser.IsValid())
		{
			SequenceBrowser.Pin()->SelectAsset(InAnimAsset);
		}

		// let the asset family know too
		TSharedRef<IAssetFamily> AssetFamily = PersonaModule.CreatePersonaAssetFamily(InAnimAsset);
		AssetFamily->RecordAssetOpened(FAssetData(InAnimAsset));
	}

	return OpenedTab;
}

void MyAnimationEditor::EditCurves(UAnimSequenceBase* InAnimSequence, const TArray<FCurveEditInfo>& InCurveInfo, const TSharedPtr<ITimeSliderController>& InExternalTimeSliderController)
{
	//FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");

	if (!AnimCurveDocumentTab.IsValid())
	{
		//TSharedRef<IAnimSequenceCurveEditor> NewCurveEditor = PersonaModule.CreateCurveWidgetForAnimDocument(SharedThis(this), GetPersonaToolkit()->GetPreviewScene(), InAnimSequence, InExternalTimeSliderController, TabManager);

		TSharedRef<MyAnimSequenceCurveEditor> NewCurveEditor = SNew(MyAnimSequenceCurveEditor, GetPersonaToolkit()->GetPreviewScene(), InAnimSequence)
			.ExternalTimeSliderController(InExternalTimeSliderController)
			.TabManager(TabManager);

		CurveEditor = NewCurveEditor;

		TSharedPtr<SDockTab> CurveTab = SNew(SDockTab)
			.Label(LOCTEXT("CurveEditorTabTitle", "Curve Editor"))
			.TabRole(ETabRole::DocumentTab)
			.TabColorScale(GetTabColorScale())
			[
				NewCurveEditor
			];

		AnimCurveDocumentTab = CurveTab;

		TabManager->InsertNewDocumentTab(AnimationEditorTabs::CurveEditorTab, FTabManager::ESearchPreference::RequireClosedTab, CurveTab.ToSharedRef());
	}
	else
	{
		TabManager->DrawAttention(AnimCurveDocumentTab.Pin().ToSharedRef());
	}

	check(CurveEditor.IsValid());

	CurveEditor.Pin()->ResetCurves();

	for (const FCurveEditInfo& CurveInfo : InCurveInfo)
	{
		CurveEditor.Pin()->AddCurve(CurveInfo.CurveDisplayName, CurveInfo.CurveColor, CurveInfo.Name, CurveInfo.Type, CurveInfo.CurveIndex, CurveInfo.OnCurveModified);
		CurveName = CurveInfo.Name.DisplayName;
	}

	CurveEditor.Pin()->ZoomToFit();
}

void MyAnimationEditor::StopEditingCurves(const TArray<FCurveEditInfo>& InCurveInfo)
{
	if (CurveEditor.IsValid())
	{
		for (const FCurveEditInfo& CurveInfo : InCurveInfo)
		{
			CurveEditor.Pin()->RemoveCurve(CurveInfo.Name, CurveInfo.Type, CurveInfo.CurveIndex);
		}
	}
}

void MyAnimationEditor::HandleSectionsChanged()
{
	OnSectionsChanged.Broadcast();
}

void MyAnimationEditor::SetAnimationAsset(UAnimationAsset* AnimAsset)
{
	HandleOpenNewAsset(AnimAsset);
}

IAnimationSequenceBrowser* MyAnimationEditor::GetAssetBrowser() const
{
	return SequenceBrowser.Pin().Get();
}

void MyAnimationEditor::HandleOpenNewAsset(UObject* InNewAsset)
{
	if (UAnimationAsset* NewAnimationAsset = Cast<UAnimationAsset>(InNewAsset))
	{
		OpenNewAnimationDocumentTab(NewAnimationAsset);
	}
}

UObject* MyAnimationEditor::HandleGetAsset()
{
	return GetEditingObject();
}

bool MyAnimationEditor::HasValidAnimationSequence() const
{
	UAnimSequence* AnimSequence = Cast<UAnimSequence>(AnimationAsset);
	return (AnimSequence != nullptr);
}

bool MyAnimationEditor::CanSetKey() const
{
	UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
	return (HasValidAnimationSequence() && PreviewMeshComponent->BonesOfInterest.Num() > 0);
}

void MyAnimationEditor::OnSetKey()
{
	if (AnimationAsset)
	{
		UDebugSkelMeshComponent* Component = PersonaToolkit->GetPreviewMeshComponent();
		Component->PreviewInstance->SetKey();
	}
}

bool MyAnimationEditor::CanApplyRawAnimChanges() const
{
	UAnimSequence* AnimSequence = Cast<UAnimSequence>(AnimationAsset);

	// ideally would be great if we can only show if something changed
	return (AnimSequence && (AnimSequence->DoesNeedRebake() || AnimSequence->DoesNeedRecompress()));
}

void MyAnimationEditor::OnApplyRawAnimChanges()
{
	UAnimSequence* AnimSequence = Cast<UAnimSequence>(AnimationAsset);
	if (AnimSequence)
	{
		if (AnimSequence->DoesNeedRebake() || AnimSequence->DoesNeedRecompress())
		{
			FScopedTransaction ScopedTransaction(LOCTEXT("BakeAnimation", "Bake Animation"));
			if (AnimSequence->DoesNeedRebake())
			{
				AnimSequence->Modify(true);
				AnimSequence->BakeTrackCurvesToRawAnimation();
			}

			if (AnimSequence->DoesNeedRecompress())
			{
				AnimSequence->Modify(true);
				AnimSequence->RequestSyncAnimRecompression(false);
			}
		}
	}
}

void MyAnimationEditor::OnReimportAnimation()
{
	UAnimSequence* AnimSequence = Cast<UAnimSequence>(AnimationAsset);
	if (AnimSequence)
	{
		FReimportManager::Instance()->Reimport(AnimSequence, true);
	}
}

void MyAnimationEditor::OnApplyCompression()
{
	UAnimSequence* AnimSequence = Cast<UAnimSequence>(AnimationAsset);
	if (AnimSequence)
	{
		TArray<TWeakObjectPtr<UAnimSequence>> AnimSequences;
		AnimSequences.Add(AnimSequence);
		FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");
		PersonaModule.ApplyCompression(AnimSequences, false);
	}
}

void MyAnimationEditor::OnExportToFBX(const EExportSourceOption Option)
{
	UAnimSequence* AnimSequenceToRecord = nullptr;
	if (Option == EExportSourceOption::CurrentAnimation_AnimData)
	{
		TArray<UObject*> AssetsToExport;
		AssetsToExport.Add(AnimationAsset);
		ExportToFBX(AssetsToExport, false);
	}
	else if (Option == EExportSourceOption::CurrentAnimation_PreviewMesh)
	{
		TArray<TWeakObjectPtr<UObject>> Skeletons;
		Skeletons.Add(PersonaToolkit->GetSkeleton());
		AnimationEditorUtils::CreateAnimationAssets(Skeletons, UAnimSequence::StaticClass(), FString("_PreviewMesh"), FAnimAssetCreated::CreateSP(this, &MyAnimationEditor::ExportToFBX, true), AnimationAsset, true);
	}
	else
	{
		ensure(false);
	}
}

bool MyAnimationEditor::ExportToFBX(const TArray<UObject*> AssetsToExport, bool bRecordAnimation)
{
	bool AnimSequenceExportResult = false;
	TArray<TWeakObjectPtr<UAnimSequence>> AnimSequences;
	if (AssetsToExport.Num() > 0)
	{
		UAnimSequence* AnimationToRecord = Cast<UAnimSequence>(AssetsToExport[0]);
		if (AnimationToRecord)
		{
			if (bRecordAnimation)
			{
				USkeletalMeshComponent* MeshComponent = PersonaToolkit->GetPreviewMeshComponent();
				RecordMeshToAnimation(MeshComponent, AnimationToRecord);
			}

			AnimSequences.Add(AnimationToRecord);
		}
	}

	if (AnimSequences.Num() > 0)
	{
		FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");


		AnimSequenceExportResult = PersonaModule.ExportToFBX(AnimSequences, GetPersonaToolkit()->GetPreviewScene()->GetPreviewMeshComponent()->SkeletalMesh);
	}
	return AnimSequenceExportResult;
}

void MyAnimationEditor::OnAddLoopingInterpolation()
{
	UAnimSequence* AnimSequence = Cast<UAnimSequence>(AnimationAsset);
	if (AnimSequence)
	{
		TArray<TWeakObjectPtr<UAnimSequence>> AnimSequences;
		AnimSequences.Add(AnimSequence);
		FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");
		PersonaModule.AddLoopingInterpolation(AnimSequences);
	}
}

void MyAnimationEditor::OnRemoveBoneTrack()
{
	if (FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("WarningOnRemovingBoneTracks", "This will clear all bone transform of the animation, source data, and edited layer information. This doesn't remove notifies, and curves. Do you want to continue?")) == EAppReturnType::Yes)
	{
		FScopedTransaction ScopedTransaction(LOCTEXT("RemoveAnimation", "Remove Track"));

		UAnimSequence* AnimSequence = Cast<UAnimSequence>(AnimationAsset);
		if (AnimSequence)
		{
			AnimSequence->Modify();
			AnimSequence->RemoveAllTracks();
		}
	}
}

TSharedRef< SWidget > MyAnimationEditor::GenerateExportAssetMenu() const
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, GetToolkitCommands());
	FillExportAssetMenu(MenuBuilder);
	return MenuBuilder.MakeWidget();
}

void MyAnimationEditor::FillCopyToSoundWaveMenu(FMenuBuilder& MenuBuilder) const
{
	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter.ClassNames.Add(*USoundWave::StaticClass()->GetName());
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateRaw(this, &MyAnimationEditor::CopyCurveToSoundWave);
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	MenuBuilder.AddWidget(
		SNew(SBox)
		.WidthOverride(300.0f)
		.HeightOverride(300.0f)
		[
			ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
		],
		FText::GetEmpty()
	);

}

void MyAnimationEditor::FillExportAssetMenu(FMenuBuilder& MenuBuilder) const
{
	MenuBuilder.BeginSection("AnimationExport", LOCTEXT("ExportAssetMenuHeading", "Export"));
	{
		MenuBuilder.AddMenuEntry(FAnimationEditorCommands::Get().ExportToFBX_AnimData);
		MenuBuilder.AddMenuEntry(FAnimationEditorCommands::Get().ExportToFBX_PreviewMesh);
	}
	MenuBuilder.EndSection();
}

FRichCurve* FindOrAddCurve(UCurveTable* CurveTable, FName CurveName)
{
	// Grab existing curve (if present)
	FRichCurve* Curve = CurveTable->FindRichCurve(CurveName, FString());
	if (Curve == nullptr)
	{
		// Or allocate new curve
		Curve = &CurveTable->AddRichCurve(CurveName);
	}

	return Curve;
}

void MyAnimationEditor::CopyCurveToSoundWave(const FAssetData& SoundWaveAssetData) const
{
	USoundWave* SoundWave = Cast<USoundWave>(SoundWaveAssetData.GetAsset());
	UAnimSequence* Sequence = Cast<UAnimSequence>(AnimationAsset);

	if (!SoundWave || !Sequence)
	{
		return;
	}

	// If no internal table, create one now
	if (!SoundWave->GetInternalCurveData())
	{
		static const FName InternalCurveTableName("InternalCurveTable");
		UCurveTable* NewCurves = NewObject<UCurveTable>(SoundWave, InternalCurveTableName);
		NewCurves->ClearFlags(RF_Public);
		NewCurves->SetFlags(NewCurves->GetFlags() | RF_Standalone | RF_Transactional);
		SoundWave->SetCurveData(NewCurves);
		SoundWave->SetInternalCurveData(NewCurves);
	}

	UCurveTable* CurveTable = SoundWave->GetInternalCurveData();

	// iterate over curves in anim data
	const int32 NumCurves = Sequence->RawCurveData.FloatCurves.Num();
	for (int32 CurveIdx = 0; CurveIdx < NumCurves; CurveIdx++)
	{
		FFloatCurve& AnimCurve = Sequence->RawCurveData.FloatCurves[CurveIdx];

		FRichCurve* Curve = FindOrAddCurve(CurveTable, AnimCurve.Name.DisplayName);
		*Curve = AnimCurve.FloatCurve; // copy data
	}

	// we will need to add a curve to tell us the time we want to start playing audio
	float PreRollTime = 0.f;
	static const FName AudioCurveName("Audio");
	FRichCurve* AudioCurve = FindOrAddCurve(CurveTable, AudioCurveName);
	AudioCurve->Reset();
	AudioCurve->AddKey(PreRollTime, 1.0f);

	// Mark dirty after 
	SoundWave->MarkPackageDirty();

	FNotificationInfo Notification(FText::Format(LOCTEXT("AddedClassSuccessNotification", "Copied curves to {0}"), FText::FromString(SoundWave->GetName())));
	FSlateNotificationManager::Get().AddNotification(Notification);

	// Close menu after picking sound
	FSlateApplication::Get().DismissAllMenus();
}

void MyAnimationEditor::ConditionalRefreshEditor(UObject* InObject)
{
	bool bInterestingAsset = true;

	if (InObject != GetPersonaToolkit()->GetSkeleton() && (GetPersonaToolkit()->GetSkeleton() && InObject != GetPersonaToolkit()->GetSkeleton()->GetPreviewMesh()) && Cast<UAnimationAsset>(InObject) != AnimationAsset)
	{
		bInterestingAsset = false;
	}

	// Check that we aren't a montage that uses an incoming animation
	if (UAnimMontage* Montage = Cast<UAnimMontage>(AnimationAsset))
	{
		for (FSlotAnimationTrack& Slot : Montage->SlotAnimTracks)
		{
			if (bInterestingAsset)
			{
				break;
			}

			for (FAnimSegment& Segment : Slot.AnimTrack.AnimSegments)
			{
				if (Segment.AnimReference == InObject)
				{
					bInterestingAsset = true;
					break;
				}
			}
		}
	}

	if (GetPersonaToolkit()->GetSkeleton() == nullptr)
	{
		bInterestingAsset = false;
	}

	if (bInterestingAsset)
	{
		GetPersonaToolkit()->GetPreviewScene()->InvalidateViews();
		OpenNewAnimationDocumentTab(Cast<UAnimationAsset>(InObject));
	}
}

void MyAnimationEditor::HandlePostReimport(UObject* InObject, bool bSuccess)
{
	if (bSuccess)
	{
		ConditionalRefreshEditor(InObject);
	}
}

void MyAnimationEditor::HandlePostImport(UFactory* InFactory, UObject* InObject)
{
	ConditionalRefreshEditor(InObject);
}

void MyAnimationEditor::HandleAnimationSequenceBrowserCreated(const TSharedRef<IAnimationSequenceBrowser>& InSequenceBrowser)
{
	SequenceBrowser = InSequenceBrowser;
}

bool MyAnimationEditor::RecordMeshToAnimation(USkeletalMeshComponent* PreviewComponent, UAnimSequence* NewAsset) const
{
	ISequenceRecorder& RecorderModule = FModuleManager::Get().LoadModuleChecked<ISequenceRecorder>("SequenceRecorder");
	return RecorderModule.RecordSingleNodeInstanceToAnimation(PreviewComponent, NewAsset);
}

#undef LOCTEXT_NAMESPACE
