
#include "MorphViewportClient.h"
#include "Morph_Viewport.h"
#include "MorphEditor.h"
#include "AdvancedPreviewScene.h"

#include "EditorStyleSet.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "AssetViewerSettings.h"


FMorphViewportClient::FMorphViewportClient(TWeakPtr<class FMorphEditor> ParentMorphEditor, const TSharedRef<class FAdvancedPreviewScene>& AdvPreviewScene, const TSharedRef<class SMorph_Viewport>& MorphViewport, UAnimationAsset* ObjectToEdit)
	: FEditorViewportClient(nullptr, &AdvPreviewScene.Get(), StaticCastSharedRef<SEditorViewport>(MorphViewport))
	, MorphEditorPtr(ParentMorphEditor)
	, MorphEditorViewportPtr(MorphViewport)
	, MorphObject(ObjectToEdit)
{
	SetViewMode(VMI_Lit);

	AdvancedPreviewScene = static_cast<FAdvancedPreviewScene*>(PreviewScene);

	SetViewLocation(FVector(0.0f, 3.0f, 2.0f));
	SetViewRotation(FRotator(-45.0f, -90.0f, 0.0f));
	SetViewLocationForOrbiting(FVector::ZeroVector, 500.0f);
}

FMorphViewportClient::~FMorphViewportClient()
{
}

void FMorphViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);
}

void FMorphViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);
}
