#include "MorphViewerPluginStyle.h"

#include "Slate/SlateGameResources.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr< FSlateStyleSet > FMorphViewerPluginStyle::StyleInstance = nullptr;

void FMorphViewerPluginStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FMorphViewerPluginStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

void FMorphViewerPluginStyle::ReloadTextures()
{
	FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}

const ISlateStyle& FMorphViewerPluginStyle::Get()
{
	return *StyleInstance;
}

FName FMorphViewerPluginStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("MorphViewerPluginStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef<class FSlateStyleSet> FMorphViewerPluginStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("MorphViewerPluginStyle"));
	Style->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));

	// Toolbar Icons
	Style->Set("MorphViewerPlugin.Command1", new IMAGE_BRUSH("Icons/icon_Landscape_Tool_Erosion_40x", Icon40x40));
	Style->Set("ToolbarIcon.Command1.Small", new IMAGE_BRUSH("Icons/icon_Landscape_Tool_Erosion_20x", Icon20x20));
	Style->Set("MorphViewerPlugin.Command2", new IMAGE_BRUSH("Icons/icon_Landscape_Tool_Flatten_40x", Icon40x40));
	Style->Set("ToolbarIcon.Command3", new IMAGE_BRUSH("Icons/icon_Landscape_Tool_Noise_40x", Icon40x40));
	Style->Set("ToolbarIcon.Command4", new IMAGE_BRUSH("Icons/icon_Landscape_Tool_Smooth_40x", Icon40x40));

	Style->Set("IGCExtensions.Command1", new IMAGE_BRUSH("Icons/icon_file_switch_16px", Icon16x16));
	Style->Set("IGCExtensions.Command2", new IMAGE_BRUSH("Icons/icon_file_savelevels_16px", Icon16x16));
	Style->Set("IGCExtensions.Command3", new IMAGE_BRUSH("Icons/icon_file_ProjectOpen_16x", Icon16x16));
	Style->Set("IGCExtensions.Command4", new IMAGE_BRUSH("Icons/icon_file_ProjectsRecent_16px", Icon16x16));

	return Style;
}
#undef IMAGE_BRUSH