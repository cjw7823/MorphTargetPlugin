#pragma once

#include "CoreMinimal.h"
#include "EditorViewportClient.h"

class FMorphViewportClient : public FEditorViewportClient, public TSharedFromThis<FMorphViewportClient>
{
public:
	// 생성자에서 모든 필요한 기본 변수를 설정.
	FMorphViewportClient(TWeakPtr<class FMorphEditor> ParentMorphEditor, const TSharedRef<class FAdvancedPreviewScene>& AdvPreviewScene, const TSharedRef<class SMorph_Viewport>& MorphViewport, UAnimationAsset* ObjectToEdit);
	~FMorphViewportClient();

	// 에디터에서 사용하는 입력
	virtual void Tick(float DeltaSeconds) override;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;


private:
	TWeakPtr<class FMorphEditor> MorphEditorPtr;
	TWeakPtr<class SMorph_Viewport> MorphEditorViewportPtr;
	UAnimationAsset* MorphObject;
	class FAdvancedPreviewScene* AdvancedPreviewScene;

};