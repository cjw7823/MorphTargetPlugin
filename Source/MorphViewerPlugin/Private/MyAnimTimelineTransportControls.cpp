// Fill out your copyright notice in the Description page of Project Settings.


#include "MyAnimTimelineTransportControls.h"
#include "EditorWidgetsModule.h"

#include "PersonaModule.h"
#include "Persona/Private/AnimationEditorPreviewScene.h"

#include "Animation/DebugSkelMeshComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimSequence.h"
#include "AnimPreviewInstance.h"
#include "Modules/ModuleManager.h"

void MyAnimTimelineTransportControls::Construct(const FArguments& InArgs, const TSharedRef<IPersonaPreviewScene>& InPreviewScene, UAnimSequenceBase* InAnimSequenceBase)
{
	WeakPreviewScene = InPreviewScene;
	AnimSequenceBase = InAnimSequenceBase;

	check(AnimSequenceBase);

	FEditorWidgetsModule& EditorWidgetsModule = FModuleManager::LoadModuleChecked<FEditorWidgetsModule>("EditorWidgets");

	FTransportControlArgs TransportControlArgs;
	TransportControlArgs.OnForwardPlay = FOnClicked::CreateSP(this, &MyAnimTimelineTransportControls::OnClick_Forward);
	TransportControlArgs.OnRecord = FOnClicked::CreateSP(this, &MyAnimTimelineTransportControls::OnClick_Record);
	TransportControlArgs.OnBackwardPlay = FOnClicked::CreateSP(this, &MyAnimTimelineTransportControls::OnClick_Backward);
	TransportControlArgs.OnForwardStep = FOnClicked::CreateSP(this, &MyAnimTimelineTransportControls::OnClick_Forward_Step);
	TransportControlArgs.OnBackwardStep = FOnClicked::CreateSP(this, &MyAnimTimelineTransportControls::OnClick_Backward_Step);
	TransportControlArgs.OnForwardEnd = FOnClicked::CreateSP(this, &MyAnimTimelineTransportControls::OnClick_Forward_End);
	TransportControlArgs.OnBackwardEnd = FOnClicked::CreateSP(this, &MyAnimTimelineTransportControls::OnClick_Backward_End);
	TransportControlArgs.OnToggleLooping = FOnClicked::CreateSP(this, &MyAnimTimelineTransportControls::OnClick_ToggleLoop);
	TransportControlArgs.OnGetLooping = FOnGetLooping::CreateSP(this, &MyAnimTimelineTransportControls::IsLoopStatusOn);
	TransportControlArgs.OnGetPlaybackMode = FOnGetPlaybackMode::CreateSP(this, &MyAnimTimelineTransportControls::GetPlaybackMode);
	TransportControlArgs.OnGetRecording = FOnGetRecording::CreateSP(this, &MyAnimTimelineTransportControls::IsRecording);

	ChildSlot
		[
			EditorWidgetsModule.CreateTransportControl(TransportControlArgs)
		];
}

UAnimSingleNodeInstance* MyAnimTimelineTransportControls::GetPreviewInstance() const
{
	UDebugSkelMeshComponent* PreviewMeshComponent = GetPreviewScene()->GetPreviewMeshComponent();
	return PreviewMeshComponent && PreviewMeshComponent->IsPreviewOn() ? PreviewMeshComponent->PreviewInstance : nullptr;
}

FReply MyAnimTimelineTransportControls::OnClick_Forward_Step()
{
	UDebugSkelMeshComponent* SMC = GetPreviewScene()->GetPreviewMeshComponent();

	if (UAnimSingleNodeInstance* PreviewInstance = GetPreviewInstance())
	{
		bool bShouldStepCloth = FMath::Abs(PreviewInstance->GetLength() - PreviewInstance->GetCurrentTime()) > SMALL_NUMBER;

		PreviewInstance->SetPlaying(false);
		PreviewInstance->StepForward();

		if (SMC && bShouldStepCloth)
		{
			SMC->bPerformSingleClothingTick = true;
		}
	}
	else if (SMC)
	{
		UAnimSequence* AnimSequence = Cast<UAnimSequence>(AnimSequenceBase);
		const float TargetFramerate = AnimSequence ? AnimSequence->GetFrameRate() : 30.0f;

		// Advance a single frame, leaving it paused afterwards
		SMC->GlobalAnimRateScale = 1.0f;
		SMC->TickAnimation(1.0f / TargetFramerate, false);
		SMC->GlobalAnimRateScale = 0.0f;
	}

	return FReply::Handled();
}

FReply MyAnimTimelineTransportControls::OnClick_Forward_End()
{
	UAnimSingleNodeInstance* PreviewInstance = GetPreviewInstance();
	if (PreviewInstance)
	{
		PreviewInstance->SetPlaying(false);
		PreviewInstance->SetPosition(PreviewInstance->GetLength(), false);
	}

	return FReply::Handled();
}

FReply MyAnimTimelineTransportControls::OnClick_Backward_Step()
{
	UAnimSingleNodeInstance* PreviewInstance = GetPreviewInstance();
	UDebugSkelMeshComponent* SMC = GetPreviewScene()->GetPreviewMeshComponent();
	if (PreviewInstance)
	{
		bool bShouldStepCloth = PreviewInstance->GetCurrentTime() > SMALL_NUMBER;

		PreviewInstance->SetPlaying(false);
		PreviewInstance->StepBackward();

		if (SMC && bShouldStepCloth)
		{
			SMC->bPerformSingleClothingTick = true;
		}
	}
	return FReply::Handled();
}

FReply MyAnimTimelineTransportControls::OnClick_Backward_End()
{
	UAnimSingleNodeInstance* PreviewInstance = GetPreviewInstance();
	if (PreviewInstance)
	{
		PreviewInstance->SetPlaying(false);
		PreviewInstance->SetPosition(0.f, false);
	}
	return FReply::Handled();
}

FReply MyAnimTimelineTransportControls::OnClick_Forward()
{
	UAnimSingleNodeInstance* PreviewInstance = GetPreviewInstance();
	UDebugSkelMeshComponent* SMC = GetPreviewScene()->GetPreviewMeshComponent();

	if (PreviewInstance)
	{
		bool bIsReverse = PreviewInstance->IsReverse();
		bool bIsPlaying = PreviewInstance->IsPlaying();
		// if current bIsReverse and bIsPlaying, we'd like to just turn off reverse
		if (bIsReverse && bIsPlaying)
		{
			PreviewInstance->SetReverse(false);
		}
		// already playing, simply pause
		else if (bIsPlaying)
		{
			PreviewInstance->SetPlaying(false);

			if (SMC && SMC->bPauseClothingSimulationWithAnim)
			{
				SMC->SuspendClothingSimulation();
			}
		}
		// if not playing, play forward
		else
		{
			//if we're at the end of the animation, jump back to the beginning before playing
			if (PreviewInstance->GetCurrentTime() >= AnimSequenceBase->GetPlayLength())
			{
				PreviewInstance->SetPosition(0.0f, false);
			}

			PreviewInstance->SetReverse(false);
			PreviewInstance->SetPlaying(true);

			if (SMC && SMC->bPauseClothingSimulationWithAnim)
			{
				SMC->ResumeClothingSimulation();
			}
		}
	}
	else if (SMC)
	{
		SMC->GlobalAnimRateScale = (SMC->GlobalAnimRateScale > 0.0f) ? 0.0f : 1.0f;
	}

	return FReply::Handled();
}

FReply MyAnimTimelineTransportControls::OnClick_Backward()
{
	UAnimSingleNodeInstance* PreviewInstance = GetPreviewInstance();
	if (PreviewInstance)
	{
		bool bIsReverse = PreviewInstance->IsReverse();
		bool bIsPlaying = PreviewInstance->IsPlaying();
		// if currently playing forward, just simply turn on reverse
		if (!bIsReverse && bIsPlaying)
		{
			PreviewInstance->SetReverse(true);
		}
		else if (bIsPlaying)
		{
			PreviewInstance->SetPlaying(false);
		}
		else
		{
			//if we're at the beginning of the animation, jump back to the end before playing
			if (PreviewInstance->GetCurrentTime() <= 0.0f)
			{
				PreviewInstance->SetPosition(AnimSequenceBase->GetPlayLength(), false);
			}

			PreviewInstance->SetPlaying(true);
			PreviewInstance->SetReverse(true);
		}
	}
	return FReply::Handled();
}

FReply MyAnimTimelineTransportControls::OnClick_ToggleLoop()
{
	UAnimSingleNodeInstance* PreviewInstance = GetPreviewInstance();
	if (PreviewInstance)
	{
		bool bIsLooping = PreviewInstance->IsLooping();
		PreviewInstance->SetLooping(!bIsLooping);
	}
	return FReply::Handled();
}

FReply MyAnimTimelineTransportControls::OnClick_Record()
{
	auto temp = StaticCastSharedRef<FAnimationEditorPreviewScene>(GetPreviewScene());
	//temp->RecordAnimation();

	bool bInRecording = false;
	FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>(TEXT("Persona"));
	PersonaModule.OnIsRecordingActive().ExecuteIfBound(temp->GetPreviewMeshComponent(), bInRecording);

	if (bInRecording)
	{
		PersonaModule.OnStopRecording().ExecuteIfBound(temp->GetPreviewMeshComponent());
	}
	else
	{
		PersonaModule.OnRecord().ExecuteIfBound(temp->GetPreviewMeshComponent());
	}

	temp->OnRecordingStateChanged().Broadcast();

	return FReply::Handled();
}

bool MyAnimTimelineTransportControls::IsLoopStatusOn() const
{
	UAnimSingleNodeInstance* PreviewInstance = GetPreviewInstance();
	return (PreviewInstance && PreviewInstance->IsLooping());
}

EPlaybackMode::Type MyAnimTimelineTransportControls::GetPlaybackMode() const
{
	if (UAnimSingleNodeInstance* PreviewInstance = GetPreviewInstance())
	{
		if (PreviewInstance->IsPlaying())
		{
			return PreviewInstance->IsReverse() ? EPlaybackMode::PlayingReverse : EPlaybackMode::PlayingForward;
		}
		return EPlaybackMode::Stopped;
	}
	else if (UDebugSkelMeshComponent* SMC = GetPreviewScene()->GetPreviewMeshComponent())
	{
		return (SMC->GlobalAnimRateScale > 0.0f) ? EPlaybackMode::PlayingForward : EPlaybackMode::Stopped;
	}

	return EPlaybackMode::Stopped;
}

bool MyAnimTimelineTransportControls::IsRecording() const
{
	auto temp = StaticCastSharedRef<FAnimationEditorPreviewScene>(GetPreviewScene());
	//temp->IsRecording();

	bool bInRecording = false;
	FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");
	PersonaModule.OnIsRecordingActive().ExecuteIfBound(temp->GetPreviewMeshComponent(), bInRecording);

	return bInRecording;
}