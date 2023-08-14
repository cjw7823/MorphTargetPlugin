// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/IToolkitHost.h"

class FMorphEditor : public FAssetEditorToolkit 
{
public:
	// 소멸자.
	virtual ~FMorphEditor();

	// 초기화 함수. 
	void InitMorphEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UAnimationAsset* InAnimationAsset);

	// IToolkit에서 상속받아 구현해야 할 가상함수들.
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetDocumentationLink() const override
	{
		return TEXT("NotAvailable");
	}

	void HandleSelectionChanged(const TArrayView<TSharedPtr<class ISkeletonTreeItem>>& InSelectedItems, ESelectInfo::Type InSelectInfo);

private:
	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Detail(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_PreviewSceneSettings(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_SkeletonTree(const FSpawnTabArgs& Args);

private:
	// IGC 에디터가 사용할 고유한 앱의 명칭.
	static const FName MorphEditorAppIdentifier;

	// 각 Tab이 사용할 고유 명칭.
	static const FName ViewportTabId;
	static const FName DetailTabId;
	static const FName PreviewSceneSettingsTabId;
	static const FName MorphSkeletonTreeId;

	// 디테일 뷰.
	TSharedPtr<class IDetailsView> DetailsView;

	// 프리뷰 위젯
	TSharedPtr<class SMorph_Viewport> Viewport;
	// 테스트 위젯
	TSharedPtr<SWidget> panel;

	// 편집할 오브젝트
	UAnimationAsset* MorphObject;

	// 프리뷰 씬 세팅 위젯.
	TSharedPtr<SWidget> AdvancedPreviewSettingsWidget;

	/** Skeleton tree */
	TSharedPtr<class ISkeletonTree> SkeletonTree;

	/** Persona toolkit */
	TSharedPtr<class IPersonaToolkit> PersonaToolkit;

	TSharedPtr<class SAnimCurveViewer> AnimCurveViewer;
};
