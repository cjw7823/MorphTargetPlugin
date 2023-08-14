// Copyright Epic Games, Inc. All Rights Reserved.

#include "MorphViewerPlugin.h"

//디테일
#include "DetailCustomization.h"

//이득우
#include "MorphViewerPluginCommands.h"
#include "MorphViewerPluginStyle.h"
#include "LevelEditor.h"
#include "IAnimationEditorModule.h"

#include "MyAnimationEditor.h"

DEFINE_LOG_CATEGORY(MorphViewerPlugin);

#define LOCTEXT_NAMESPACE "FMorphViewerPluginModule"

void FMorphViewerPluginModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	//디테일 탭 수정. 언리얼 Documentation 예제 실습.
	/*FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(FName(*USkeletalMeshComponent::StaticClass()->GetName()), FOnGetDetailCustomizationInstance::CreateStatic(&FDetailCustomization::MakeInstance));*/

	//----------------------------------------------------------
	// 스타일 등록.
	FMorphViewerPluginStyle::Initialize();
	FMorphViewerPluginStyle::ReloadTextures();
	// 커맨드 등록.
	FMorphViewerPluginCommands::Register();

	// 커맨드와 액션을 서로 묶어주기.
	CommandList = MakeShareable(new FUICommandList());
	CommandList->MapAction(FMorphViewerPluginCommands::Get().Command1, FExecuteAction::CreateStatic(&FMorphViewerPluginActions::Action1), FCanExecuteAction());
	CommandList->MapAction(FMorphViewerPluginCommands::Get().Command2, FExecuteAction::CreateStatic(&FMorphViewerPluginActions::Action2), FCanExecuteAction());
	
	// 애님 에디터 얻어오기. 
	IAnimationEditorModule& AnimEditorModule = FModuleManager::LoadModuleChecked<IAnimationEditorModule>("AnimationEditor");

	// 툴바 생성을 위한 델리게이트 함수 선언.
	struct CustomToolbar
	{
		static void CreateCustomToolbar(FToolBarBuilder& ToolbarBuilder)
		{
			ToolbarBuilder.BeginSection("CustomToolbar");
			{
				ToolbarBuilder.AddToolBarButton(FMorphViewerPluginCommands::Get().Command1);
				//ToolbarBuilder.AddToolBarButton(FMorphViewerPluginCommands::Get().Command2);
			}
			ToolbarBuilder.EndSection();
		}
	};

	FName levelEditorName = FName("Settings");
	FName animationEditorName = FName("Editing");

	// 툴바 생성.
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender());
	ToolbarExtender->AddToolBarExtension(animationEditorName /*툴바를삽입할위치*/, EExtensionHook::After, CommandList,
		FToolBarExtensionDelegate::CreateStatic(&CustomToolbar::CreateCustomToolbar));

	// 툴바 추가.
	AnimEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
}

void FMorphViewerPluginModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomClassLayout(FName(*USkeletalMeshComponent::StaticClass()->GetName()));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMorphViewerPluginModule, MorphViewerPlugin)