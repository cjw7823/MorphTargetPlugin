// Fill out your copyright notice in the Description page of Project Settings.


#include "DetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Components/SkeletalMeshComponent.h"

FDetailCustomization::FDetailCustomization()
{
}

FDetailCustomization::~FDetailCustomization()
{
}

TSharedRef<IDetailCustomization> FDetailCustomization::MakeInstance()
{
	return MakeShareable(new FDetailCustomization);
}

void FDetailCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& LightingCategory = DetailBuilder.EditCategory("Lighting", FText::FromString(TEXT("OptionalLocalizedDisplayNam")));

	LightingCategory.AddCustomRow(FText::FromString(TEXT("MyCustom"))).NameContent()[
		SNew(STextBlock).Text(FText::FromString("dkdkdkdkdk"))
	];

	LightingCategory.AddCustomRow(FText::FromString(TEXT("OptionalLocalizedDisplayName1111")))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("OptionalLocalizedDisplayName")))
			//.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
			[
			SNew(SButton)
			.Text(FText::FromString(TEXT("OptionalLocalizedDisplayName")))
			.OnClicked(FOnClicked::CreateLambda([]() {
				UE_LOG(LogTemp, Warning, TEXT("OptionalLocalizedDisplayName"));
				return FReply::Handled();
			}))
		];
}
