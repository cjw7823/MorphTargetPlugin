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
    //����Ʈ�� �ٽ� ������ �� �ֵ��� SpinboxList���� ����.
    auto spinbox = StaticCastSharedRef<SMySpinBox>(widgetRef.Pin()->GetChildren()->GetChildAt(1));
    funclass::GetInstance()->ViewportSpinboxList.Remove(spinbox);

    funclass::GetInstance()->MorphLayoutCanvas->RemoveSlot(widgetRef.Pin().ToSharedRef());

    return FReply::Unhandled();
}

FReply SMyButton::MyOnClicked_Reset()
{
    //MyAnimationEditor::EditCurve()�� ȣ��Ǿ� CurveEditor�� ���������� Ȯ��.
    TSharedPtr<MyAnimSequenceCurveEditor> SequenceCurveEditor = FMorphViewerPluginActions::NewAnimEditor.Pin()->GetCurveEditor().Pin();
    if (SequenceCurveEditor.IsValid())
    {
        //���� �� Ű CurveEditor�� Selection �ʱ�ȭ
        TSharedPtr<FCurveEditor> CurveEditor = SequenceCurveEditor->GetCurveEditor();
        CurveEditor->Selection.Clear();
    }

    //��� ���ɹڽ� �� 0���� �ʱ�ȭ
    for (auto spinbox : funclass::GetInstance()->SpinboxList)
    {
        spinbox->My_SpinBox.Pin()->SetValue(0.f);
    }

    return FReply::Unhandled();
}
