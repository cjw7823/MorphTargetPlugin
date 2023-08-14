// Fill out your copyright notice in the Description page of Project Settings.


#include "SMyButton.h"
#include "MorphViewerPluginCommands.h"
#include "Widgets/SCanvas.h"
#include "SMySpinBox.h"

#include "MyAnimationEditor.h"
#include "MyAnimSequenceCurveEditor.h"
#include "CurveEditor.h"

void SMyButton::Construct(const FArguments& InArgs)
{
    widgetRef = InArgs._RemovedObj;

    if (InArgs._Mode == FString("Close"))
    {
        ChildSlot
            [
                SNew(SButton)
                .Text(FText::FromString("X"))
                .OnClicked(this, &SMyButton::OnClicked_SMyButton)
            ];
    }
    else if (InArgs._Mode == FString("Reset"))
    {
        ChildSlot
            [
                SNew(SButton)
                .Text(FText::FromString("Reset"))
                .OnClicked(this, &SMyButton::MyOnClicked_Reset)
            ];
    }
}

FReply SMyButton::OnClicked_SMyButton()
{
    //뷰포트에 다시 생성할 수 있도록 SpinboxList에서 제거.
    auto spinbox = StaticCastSharedRef<SMySpinBox>(widgetRef.Pin()->GetChildren()->GetChildAt(1));
    funclass::GetInstance()->ViewportSpinboxList.Remove(spinbox);

    funclass::GetInstance()->MorphLayoutCanvas->RemoveSlot(widgetRef.Pin().ToSharedRef());

    return FReply::Unhandled();
}

FReply SMyButton::MyOnClicked_Reset()
{
    //MyAnimationEditor::EditCurve()가 호출되어 CurveEditor가 생성었는지 확인.
    TSharedPtr<MyAnimSequenceCurveEditor> SequenceCurveEditor = FMorphViewerPluginActions::NewAnimEditor.Pin()->GetCurveEditor().Pin();
    if (SequenceCurveEditor.IsValid())
    {
        //선택 된 키 CurveEditor의 Selection 초기화
        TSharedPtr<FCurveEditor> CurveEditor = SequenceCurveEditor->GetCurveEditor();
        CurveEditor->Selection.Clear();
    }

    //모든 스핀박스 값 0으로 초기화
    for (auto spinbox : funclass::GetInstance()->SpinboxList)
    {
        spinbox->My_SpinBox.Pin()->SetValue(0.f);
    }

    return FReply::Unhandled();
}
