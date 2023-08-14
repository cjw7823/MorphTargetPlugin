#pragma once

#include "CoreMinimal.h"
#include "EditorViewportClient.h"

class FMorphViewportClient : public FEditorViewportClient, public TSharedFromThis<FMorphViewportClient>
{
public:
	// �����ڿ��� ��� �ʿ��� �⺻ ������ ����.
	FMorphViewportClient(TWeakPtr<class FMorphEditor> ParentMorphEditor, const TSharedRef<class FAdvancedPreviewScene>& AdvPreviewScene, const TSharedRef<class SMorph_Viewport>& MorphViewport, UAnimationAsset* ObjectToEdit);
	~FMorphViewportClient();

	// �����Ϳ��� ����ϴ� �Է�
	virtual void Tick(float DeltaSeconds) override;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;


private:
	TWeakPtr<class FMorphEditor> MorphEditorPtr;
	TWeakPtr<class SMorph_Viewport> MorphEditorViewportPtr;
	UAnimationAsset* MorphObject;
	class FAdvancedPreviewScene* AdvancedPreviewScene;

};