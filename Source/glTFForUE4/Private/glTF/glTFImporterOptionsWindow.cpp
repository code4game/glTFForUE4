// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4PrivatePCH.h"
#include "glTF/glTFImporterOptionsWindow.h"

#include "glTF/glTFImporterOptions.h"

#include "Styling/CoreStyle.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "glTFForUE4"

TSharedPtr<FglTFImporterOptions> SglTFImporterOptionsWindow::Open(const FString& InFilePathInOS, const FString& InFilePathInEngine, const libgltf::SGlTF& InGlTF, bool& OutCancel)
{
    TSharedPtr<FglTFImporterOptions> glTFImporterOptions = MakeShareable(new FglTFImporterOptions());

    glTFImporterOptions->FilePathInOS = InFilePathInOS;
    glTFImporterOptions->FilePathInEngine = InFilePathInEngine;

    //TODO: for runtime
    return glTFImporterOptions;
}

SglTFImporterOptionsWindow::SglTFImporterOptionsWindow()
    : SCompoundWidget()
    , glTFImporterOptions(nullptr)
    , WidgetWindow(nullptr)
{
    //
}

void SglTFImporterOptionsWindow::Construct(const FArguments& InArgs)
{
    glTFImporterOptions = InArgs._glTFImporterOptions;
    checkf(glTFImporterOptions.IsValid(), TEXT("Why the argument - glTFImporterOptions is null?"));
    WidgetWindow = InArgs._WidgetWindow;

    //TODO:
}

bool SglTFImporterOptionsWindow::SupportsKeyboardFocus() const
{
    return true;
}

FReply SglTFImporterOptionsWindow::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        return OnCancel();
    }

    return FReply::Unhandled();
}

TSharedPtr<FglTFImporterOptions> SglTFImporterOptionsWindow::GetImportOptions()
{
    return glTFImporterOptions.Pin();
}

bool SglTFImporterOptionsWindow::CanImport() const
{
    return true;
}

FReply SglTFImporterOptionsWindow::OnImport()
{
    if (WidgetWindow.IsValid())
    {
        WidgetWindow.Pin()->RequestDestroyWindow();
    }
    return FReply::Unhandled();
}

FReply SglTFImporterOptionsWindow::OnCancel()
{
    glTFImporterOptions.Reset();

    if (WidgetWindow.IsValid())
    {
        WidgetWindow.Pin()->RequestDestroyWindow();
    }
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
