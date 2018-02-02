// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4PrivatePCH.h"
#include "glTF/glTFImportOptionsWindow.h"

#include "glTF/glTFImportOptions.h"

#include "libgltf/libgltf.h"

#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "FglTFForUE4EdModule"

TSharedPtr<FglTFImportOptions> SglTFImportOptionsWindow::Open(const FString& InFilePathInOS, const FString& InFilePathInEngine, const libgltf::SGlTF& InGlTF, bool& OutCancel)
{
    TSharedPtr<libgltf::SGlTF> GlTF = MakeShareable(new libgltf::SGlTF());
    (*GlTF) = InGlTF;

    TSharedPtr<FglTFImportOptions> glTFImportOptions = MakeShareable(new FglTFImportOptions());
    (*glTFImportOptions) = FglTFImportOptions::Current;

    glTFImportOptions->FilePathInOS = InFilePathInOS;
    glTFImportOptions->FilePathInEngine = InFilePathInEngine;

    TSharedPtr<SWindow> ParentWindow;

    //TODO: get the parent window

    TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(LOCTEXT("glTFImportWindowTitle", "Import glTF"))
        .SizingRule(ESizingRule::Autosized);

    TSharedPtr<SglTFImportOptionsWindow> glTFImportWindow;
    Window->SetContent
    (
        SAssignNew(glTFImportWindow, SglTFImportOptionsWindow)
            .GlTF(GlTF)
            .glTFImportOptions(glTFImportOptions)
            .WidgetWindow(Window)
    );

    /// Show the import options window.
    FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

    OutCancel = (glTFImportOptions != glTFImportWindow->GetImportOptions());

    /// Store the option if not cancel
    if (!OutCancel)
    {
        FglTFImportOptions::Current = (*glTFImportOptions);
    }
    return glTFImportOptions;
}

SglTFImportOptionsWindow::SglTFImportOptionsWindow()
    : SCompoundWidget()
    , glTFImportOptions(nullptr)
    , WidgetWindow(nullptr)
{
    //
}

void SglTFImportOptionsWindow::Construct(const FArguments& InArgs)
{
    GlTF = InArgs._GlTF;
    WidgetWindow = InArgs._WidgetWindow;
    glTFImportOptions = InArgs._glTFImportOptions;
    checkf(glTFImportOptions.IsValid(), TEXT("Why the argument - glTFImportOptions is null?"));

    //TODO:
}

bool SglTFImportOptionsWindow::SupportsKeyboardFocus() const
{
    return true;
}

FReply SglTFImportOptionsWindow::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        return OnCancel();
    }

    return FReply::Unhandled();
}

TSharedPtr<FglTFImportOptions> SglTFImportOptionsWindow::GetImportOptions()
{
    return glTFImportOptions.Pin();
}

bool SglTFImportOptionsWindow::CanImport() const
{
    return true;
}

FReply SglTFImportOptionsWindow::OnImport()
{
    if (WidgetWindow.IsValid())
    {
        WidgetWindow.Pin()->RequestDestroyWindow();
    }
    return FReply::Unhandled();
}

FReply SglTFImportOptionsWindow::OnCancel()
{
    glTFImportOptions.Reset();

    if (WidgetWindow.IsValid())
    {
        WidgetWindow.Pin()->RequestDestroyWindow();
    }
    return FReply::Handled();
}

void SglTFImportOptionsWindow::HandleImportScene(ECheckBoxState InCheckBoxState)
{
    glTFImportOptions.Pin()->bImportAsScene = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImportOptionsWindow::HandleImportSkeleton(ECheckBoxState InCheckBoxState)
{
    glTFImportOptions.Pin()->bImportAsSkeleton = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImportOptionsWindow::HandleImportMaterial(ECheckBoxState InCheckBoxState)
{
    glTFImportOptions.Pin()->bImportMaterial = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImportOptionsWindow::HandleMeshScaleRatio(float InNewValue)
{
    glTFImportOptions.Pin()->MeshScaleRatio = InNewValue;
}

void SglTFImportOptionsWindow::HandleMeshInvertNormal(ECheckBoxState InCheckBoxState)
{
    glTFImportOptions.Pin()->bInvertNormal = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImportOptionsWindow::HandleMeshUseMikkTSpace(ECheckBoxState InCheckBoxState)
{
    glTFImportOptions.Pin()->bUseMikkTSpace = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImportOptionsWindow::HandleMeshRecomputeNormals(ECheckBoxState InCheckBoxState)
{
    glTFImportOptions.Pin()->bRecomputeNormals = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImportOptionsWindow::HandleMeshRecomputeTangents(ECheckBoxState InCheckBoxState)
{
    glTFImportOptions.Pin()->bRecomputeTangents = (InCheckBoxState == ECheckBoxState::Checked);
}

#undef LOCTEXT_NAMESPACE
