// Fill out your copyright notice in the Description page of Project Settings.


#include "MyAnimSequenceCurveEditor.h"
#include "CurveEditor.h"
#include "RichCurveEditorModel.h"
#include "Animation/AnimSequenceBase.h"
#include "SCurveEditorPanel.h"
#include "Tree/SCurveEditorTreeTextFilter.h"
#include "Tree/SCurveEditorTreeFilterStatusBar.h"
#include "Tree/SCurveEditorTree.h"
#include "Tree/SCurveEditorTreeSelect.h"
#include "Tree/SCurveEditorTreePin.h"
#include "Widgets/Layout/SScrollBorder.h"
#include "Tree/ICurveEditorTreeItem.h"

#include "MyAnimTimelineTransportControls.h"
//#include "Persona/Private/AnimTimeline/SAnimTimelineTransportControls.h"

#include "Tree/CurveEditorTreeFilter.h"
#include "Editor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "MyAnimSequenceCurveEditor"

bool MyFRichCurveEditorModelNamed::IsValid() const
{
	return AnimSequence->GetCurveData().GetCurveData(Name.UID, Type) != nullptr;
}

FRichCurve& MyFRichCurveEditorModelNamed::GetRichCurve()
{
	check(AnimSequence.Get() != nullptr);

	FAnimCurveBase* CurveBase = AnimSequence->RawCurveData.GetCurveData(Name.UID, Type);
	check(CurveBase);	// If this fails lifetime contracts have been violated - this curve should always be present if this model exists

	switch (Type)
	{
	case ERawCurveTrackTypes::RCT_Vector:
	{
		FVectorCurve& VectorCurve = *(static_cast<FVectorCurve*>(CurveBase));
		check(CurveIndex < 3);
		return VectorCurve.FloatCurves[CurveIndex];
	}
	case ERawCurveTrackTypes::RCT_Transform:
	{
		FTransformCurve& TransformCurve = *(static_cast<FTransformCurve*>(CurveBase));
		check(CurveIndex < 9);
		const int32 SubCurveIndex = CurveIndex % 3;
		switch (CurveIndex)
		{
		default:
			check(false);
			// fall through
		case 0:
		case 1:
		case 2:
			return TransformCurve.TranslationCurve.FloatCurves[SubCurveIndex];
		case 3:
		case 4:
		case 5:
			return TransformCurve.RotationCurve.FloatCurves[SubCurveIndex];
		case 6:
		case 7:
		case 8:
			return TransformCurve.ScaleCurve.FloatCurves[SubCurveIndex];
		}

	}
	case ERawCurveTrackTypes::RCT_Float:
	default:
	{
		FFloatCurve& FloatCurve = *(static_cast<FFloatCurve*>(CurveBase));
		check(CurveIndex == 0);
		return FloatCurve.FloatCurve;
	}
	}
}

const FRichCurve& MyFRichCurveEditorModelNamed::GetReadOnlyRichCurve() const
{
	return const_cast<MyFRichCurveEditorModelNamed*>(this)->GetRichCurve();
}

class FAnimSequenceCurveEditorItem : public ICurveEditorTreeItem
{
public:
	FAnimSequenceCurveEditorItem(const FSmartName& InName, ERawCurveTrackTypes InType, int32 InCurveIndex, UAnimSequenceBase* InAnimSequence, const FText& InCurveDisplayName, const FLinearColor& InCurveColor, FSimpleDelegate InOnCurveModified, FCurveEditorTreeItemID InTreeId)
		: Name(InName)
		, Type(InType)
		, CurveIndex(InCurveIndex)
		, AnimSequence(InAnimSequence)
		, CurveDisplayName(InCurveDisplayName)
		, CurveColor(InCurveColor)
		, OnCurveModified(InOnCurveModified)
		, TreeId(InTreeId)
	{
	}

	virtual TSharedPtr<SWidget> GenerateCurveEditorTreeWidget(const FName& InColumnName, TWeakPtr<FCurveEditor> InCurveEditor, FCurveEditorTreeItemID InTreeItemID, const TSharedRef<ITableRow>& InTableRow) override
	{
		if (InColumnName == ColumnNames.Label)
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(FMargin(4.f))
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Right)
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(CurveDisplayName)
				.ColorAndOpacity(FSlateColor(CurveColor))
				];
		}
		else if (InColumnName == ColumnNames.SelectHeader)
		{
			return SNew(SCurveEditorTreeSelect, InCurveEditor, InTreeItemID, InTableRow);
		}
		else if (InColumnName == ColumnNames.PinHeader)
		{
			return SNew(SCurveEditorTreePin, InCurveEditor, InTreeItemID, InTableRow);
		}

		return nullptr;
	}

	virtual void CreateCurveModels(TArray<TUniquePtr<FCurveModel>>& OutCurveModels) override
	{
		TUniquePtr<MyFRichCurveEditorModelNamed> NewCurveModel = MakeUnique<MyFRichCurveEditorModelNamed>(Name, Type, CurveIndex, AnimSequence.Get(), TreeId);
		NewCurveModel->SetShortDisplayName(CurveDisplayName);
		NewCurveModel->SetLongDisplayName(CurveDisplayName);
		NewCurveModel->SetColor(CurveColor);
		NewCurveModel->OnCurveModified().Add(OnCurveModified);

		OutCurveModels.Add(MoveTemp(NewCurveModel));
	}

	virtual bool PassesFilter(const FCurveEditorTreeFilter* InFilter) const override
	{
		if (InFilter->GetType() == ECurveEditorTreeFilterType::Text)
		{
			const FCurveEditorTreeTextFilter* Filter = static_cast<const FCurveEditorTreeTextFilter*>(InFilter);
			for (const FCurveEditorTreeTextFilterTerm& Term : Filter->GetTerms())
			{
				for (const FCurveEditorTreeTextFilterToken& Token : Term.ChildToParentTokens)
				{
					if (Token.Match(*CurveDisplayName.ToString()))
					{
						return true;
					}
				}
			}

			return false;
		}

		return false;
	}

	FSmartName Name;
	ERawCurveTrackTypes Type;
	int32 CurveIndex;
	TWeakObjectPtr<UAnimSequenceBase> AnimSequence;
	FText CurveDisplayName;
	FLinearColor CurveColor;
	FSimpleDelegate OnCurveModified;
	FCurveEditorTreeItemID TreeId;
};

class FAnimSequenceCurveEditorBounds : public ICurveEditorBounds
{
public:
	FAnimSequenceCurveEditorBounds(TSharedPtr<ITimeSliderController> InExternalTimeSliderController)
		: ExternalTimeSliderController(InExternalTimeSliderController)
	{}

	virtual void GetInputBounds(double& OutMin, double& OutMax) const override
	{
		FAnimatedRange ViewRange = ExternalTimeSliderController.Pin()->GetViewRange();
		OutMin = ViewRange.GetLowerBoundValue();
		OutMax = ViewRange.GetUpperBoundValue();
	}

	virtual void SetInputBounds(double InMin, double InMax) override
	{
		ExternalTimeSliderController.Pin()->SetViewRange(InMin, InMax, EViewRangeInterpolation::Immediate);
	}

	TWeakPtr<ITimeSliderController> ExternalTimeSliderController;
};

MyAnimSequenceCurveEditor::MyAnimSequenceCurveEditor()
{
	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}
}

MyAnimSequenceCurveEditor::~MyAnimSequenceCurveEditor()
{
	if (GEditor)
	{
		GEditor->UnregisterForUndo(this);
	}
}

void MyAnimSequenceCurveEditor::Construct(const FArguments& InArgs, const TSharedRef<IPersonaPreviewScene>& InPreviewScene, UAnimSequenceBase* InAnimSequence)
{
	CurveEditor = MakeShared<FCurveEditor>();
	CurveEditor->GridLineLabelFormatXAttribute = LOCTEXT("GridXLabelFormat", "{0}s");
	CurveEditor->SetBounds(MakeUnique<FAnimSequenceCurveEditorBounds>(InArgs._ExternalTimeSliderController));

	FCurveEditorInitParams CurveEditorInitParams;
	CurveEditor->InitCurveEditor(CurveEditorInitParams);

	AnimSequence = InAnimSequence;

	CurveEditorTree = SNew(SCurveEditorTree, CurveEditor);

	TSharedRef<SCurveEditorPanel> CurveEditorPanel = SNew(SCurveEditorPanel, CurveEditor.ToSharedRef())
		.GridLineTint(FLinearColor(0.f, 0.f, 0.f, 0.3f))
		.ExternalTimeSliderController(InArgs._ExternalTimeSliderController)
		.TabManager(InArgs._TabManager)
		.TreeSplitterWidth(0.2f)
		.ContentSplitterWidth(0.8f)
		.TreeContent()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(CurveEditorSearchBox, SCurveEditorTreeTextFilter, CurveEditor)
		]
	+ SVerticalBox::Slot()
		[
			SNew(SScrollBorder, CurveEditorTree.ToSharedRef())
			[
				CurveEditorTree.ToSharedRef()
			]
		]
	+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SCurveEditorTreeFilterStatusBar, CurveEditor)
		]
	+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(MyAnimTimelineTransportControls, InPreviewScene, InAnimSequence)
		]
		];

	ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 3.0f)
		[
			MakeToolbar(CurveEditorPanel)
		]
	+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			CurveEditorPanel
		]
		];
}

TSharedRef<SWidget> MyAnimSequenceCurveEditor::MakeToolbar(TSharedRef<SCurveEditorPanel> InEditorPanel)
{
	FToolBarBuilder ToolBarBuilder(InEditorPanel->GetCommands(), FMultiBoxCustomization::None, InEditorPanel->GetToolbarExtender(), true);
	ToolBarBuilder.SetStyle(&FEditorStyle::Get(), "Sequencer.ToolBar");
	ToolBarBuilder.BeginSection("Asset");
	ToolBarBuilder.EndSection();
	// We just use all of the extenders as our toolbar, we don't have a need to create a separate toolbar.
	return ToolBarBuilder.MakeWidget();
}

void MyAnimSequenceCurveEditor::ResetCurves()
{
	CurveEditor->RemoveAllTreeItems();
	CurveEditor->RemoveAllCurves();
}

void MyAnimSequenceCurveEditor::AddCurve(const FText& InCurveDisplayName, const FLinearColor& InCurveColor, const FSmartName& InName, ERawCurveTrackTypes InType, int32 InCurveIndex, FSimpleDelegate InOnCurveModified)
{
	FCurveEditorTreeItem* TreeItem = CurveEditor->AddTreeItem(FCurveEditorTreeItemID());
	TreeItem->SetStrongItem(MakeShared<FAnimSequenceCurveEditorItem>(InName, InType, InCurveIndex, AnimSequence, InCurveDisplayName, InCurveColor, InOnCurveModified, TreeItem->GetID()));

	// Update selection
	const TMap<FCurveEditorTreeItemID, ECurveEditorTreeSelectionState>& Selection = CurveEditor->GetTreeSelection();
	TArray<FCurveEditorTreeItemID> NewSelection;
	NewSelection.Add(TreeItem->GetID());
	for (const auto& SelectionPair : Selection)
	{
		if (SelectionPair.Value != ECurveEditorTreeSelectionState::None)
		{
			NewSelection.Add(SelectionPair.Key);
		}
	}
	CurveEditor->SetTreeSelection(MoveTemp(NewSelection));
}

void MyAnimSequenceCurveEditor::RemoveCurve(const FSmartName& InName, ERawCurveTrackTypes InType, int32 InCurveIndex)
{
	for (const auto& CurvePair : CurveEditor->GetCurves())
	{
		MyFRichCurveEditorModelNamed* Model = static_cast<MyFRichCurveEditorModelNamed*>(CurvePair.Value.Get());
		if (Model->Name == InName && Model->Type == InType && Model->CurveIndex == InCurveIndex)
		{
			CurveEditor->RemoveCurve(CurvePair.Key);
			CurveEditor->RemoveTreeItem(Model->TreeId);
			break;
		}
	}
}

void MyAnimSequenceCurveEditor::ZoomToFit()
{
	CurveEditor->ZoomToFit(EAxisList::Y);
}

void MyAnimSequenceCurveEditor::PostUndoRedo()
{
	// Check if our curves are still valid & remove if not
	bool bRemoved = false;
	do
	{
		bRemoved = false;
		for (const TPair<FCurveModelID, TUniquePtr<FCurveModel>>& Pair : CurveEditor->GetCurves())
		{
			MyFRichCurveEditorModelNamed* Model = static_cast<MyFRichCurveEditorModelNamed*>(Pair.Value.Get());
			if (!Model->IsValid())
			{
				CurveEditor->RemoveCurve(Pair.Key);
				CurveEditor->RemoveTreeItem(Model->TreeId);
				bRemoved = true;
				break;
			}
		}
	} while (bRemoved);
}

#undef LOCTEXT_NAMESPACE