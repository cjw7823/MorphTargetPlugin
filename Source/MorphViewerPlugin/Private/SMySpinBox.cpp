// Fill out your copyright notice in the Description page of Project Settings.


#include "SMySpinBox.h"

#include "MorphViewerPluginCommands.h"
#include "Persona/Private/AnimationEditorPreviewScene.h"

#include "MyAnimationEditor.h"
#include "MyAnimSequenceCurveEditor.h"
#include "CurveEditor.h"
#include "Views/SCurveEditorViewAbsolute.h"
#include "CurveEditor/Public/Views/SInteractiveCurveEditorView.h"
#include "CurveEditorTypes.h"

void SMySpinBox::Construct(const FArguments& InArgs)
{
    MorphTargetName = InArgs._MorphTargetName;
    Brother_SpinBox = InArgs._Brother_SpinBox;

    float value = 0.f;

    if(Brother_SpinBox.IsValid())
        value = StaticCastSharedRef<SSpinBox<float>>(Brother_SpinBox.Pin()->GetChildren()->GetChildAt(0))->GetValue();

    ChildSlot
    [
        SAssignNew(My_SpinBox, SSpinBox<float>)
        .MinSliderValue(-1.f)
        .MaxSliderValue(1.f)
        .MinValue(-1.0)
        .MaxValue(1.0)
        .OnValueChanged(this, &SMySpinBox::OnValueChanged_SpinBox)
        .Value(value)
	];
}

void SMySpinBox::OnValueChanged_SpinBox(float value)
{
    //스핀박스의 값이 변경되면 형제 스핀박스의 값을 변경.
    if (Brother_SpinBox.IsValid())
    {
        if (!bInfiniteLoop)
        {
            Brother_SpinBox.Pin()->bInfiniteLoop = true;
            StaticCastSharedRef<SSpinBox<float>>(Brother_SpinBox.Pin()->GetChildren()->GetChildAt(0))->SetValue(value);
        }
        else
        {
			bInfiniteLoop = false;
			return;
        }
	}

    //모프타깃 이름으로 모프타깃을 찾아서 값을 설정.
    funclass::GetInstance()->PreviewScene->GetPreviewMeshComponent()->SetMorphTarget(FName(*MorphTargetName), value);

    //MyAnimationEditor::EditCurve()가 호출되어 CurveEditor가 생성었는지 확인.
    TSharedPtr<MyAnimSequenceCurveEditor> SequenceCurveEditor = FMorphViewerPluginActions::NewAnimEditor.Pin()->GetCurveEditor().Pin();
    if (SequenceCurveEditor.IsValid())
    {
        //편집할 커브와 스핀박스의 이름이 다르면 리턴.
        FString CurveName = FMorphViewerPluginActions::NewAnimEditor.Pin()->CurveName.ToString();
        if (MorphTargetName != CurveName)
            return;

        TSharedPtr<FCurveEditor> CurveEditor = SequenceCurveEditor->GetCurveEditor();
        FCurveEditorSelection selection = CurveEditor->GetSelection();
        //선택된 키가 없으면 리턴.
        if(selection.IsEmpty())
			return;

        FCurveModel* CurveToAddTo = nullptr;
        TArrayView<const FKeyHandle> keyhandles;
        for (FCurveModelID id : CurveEditor->GetEditedCurves())
        {
            CurveToAddTo = CurveEditor->FindCurve(id);
            keyhandles = selection.FindForCurve(id)->AsArray();
            //break;
        }
        TArray<FKeyPosition> MyArray;
        MyArray.Init(FKeyPosition(), keyhandles.Num());
        TArrayView<FKeyPosition> keypositions(MyArray.GetData(), MyArray.Num());

        //키 포지션의 X값을 가져오기 위해.
        CurveToAddTo->GetKeyPositions(keyhandles, keypositions);

        //Y값을 a로 변경.
        for (auto iter = keypositions.begin(); iter != keypositions.end(); ++iter)
        {
			iter->OutputValue = value;
		}

        CurveToAddTo->SetKeyPositions(keyhandles, keypositions);
	}
}
