// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/IToolkitHost.h"

class FMorphEditor : public FAssetEditorToolkit 
{
public:
	// �Ҹ���.
	virtual ~FMorphEditor();

	// �ʱ�ȭ �Լ�. 
	void InitMorphEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UAnimationAsset* InAnimationAsset);

	// IToolkit���� ��ӹ޾� �����ؾ� �� �����Լ���.
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
	// IGC �����Ͱ� ����� ������ ���� ��Ī.
	static const FName MorphEditorAppIdentifier;

	// �� Tab�� ����� ���� ��Ī.
	static const FName ViewportTabId;
	static const FName DetailTabId;
	static const FName PreviewSceneSettingsTabId;
	static const FName MorphSkeletonTreeId;

	// ������ ��.
	TSharedPtr<class IDetailsView> DetailsView;

	// ������ ����
	TSharedPtr<class SMorph_Viewport> Viewport;
	// �׽�Ʈ ����
	TSharedPtr<SWidget> panel;

	// ������ ������Ʈ
	UAnimationAsset* MorphObject;

	// ������ �� ���� ����.
	TSharedPtr<SWidget> AdvancedPreviewSettingsWidget;

	/** Skeleton tree */
	TSharedPtr<class ISkeletonTree> SkeletonTree;

	/** Persona toolkit */
	TSharedPtr<class IPersonaToolkit> PersonaToolkit;

	TSharedPtr<class SAnimCurveViewer> AnimCurveViewer;
};
