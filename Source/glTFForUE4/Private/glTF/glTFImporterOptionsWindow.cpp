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
    (*glTFImporterOptions) = FglTFImporterOptions::Current;

    glTFImporterOptions->FilePathInOS = InFilePathInOS;
    glTFImporterOptions->FilePathInEngine = InFilePathInEngine;

    /*TSharedPtr<SWindow> ParentWindow;

    //TODO: get the parent window

    TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(LOCTEXT("glTFImportWindowTitle", "Import glTF"))
        .SizingRule(ESizingRule::Autosized);

    TSharedPtr<SglTFImporterOptionsWindow> glTFImportWindow;
    Window->SetContent
    (
        SAssignNew(glTFImportWindow, SglTFImporterOptionsWindow)
            .glTFImporterOptions(glTFImporterOptions)
            .WidgetWindow(Window)
    );

    /// Show the import options window.
    FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

    OutCancel = (glTFImporterOptions != glTFImportWindow->GetImportOptions());

    /// Store the option if not cancel
    if (!OutCancel)
    {
        FglTFImporterOptions::Current = (*glTFImporterOptions);
    }*/
    return glTFImporterOptions;
}

SglTFImporterOptionsWindow::SglTFImporterOptionsWindow()
    : SCompoundWidget()
    , glTFImporterOptions(nullptr)
    , WidgetWindow(nullptr)
    , ImportTypes()
    , bHasAnimation(false)
    , bReimport(false)
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

void SglTFImporterOptionsWindow::HandleImportType(const TSharedPtr<EglTFImportType> InImportType, ESelectInfo::Type InSelectInfo)
{
    if (!InImportType.IsValid()) return;
    glTFImporterOptions.Pin()->ImportType = *InImportType;
}

TSharedRef<SWidget> SglTFImporterOptionsWindow::GenerateImportType(TSharedPtr<EglTFImportType> InImportType) const
{
    FText ImportTypeText = GetImportTypeText(InImportType.IsValid() ? *InImportType : EglTFImportType::None);
    return SNew(STextBlock)
        .Text(ImportTypeText);
}

FText SglTFImporterOptionsWindow::GetImportTypeText() const
{
    return GetImportTypeText(glTFImporterOptions.Pin()->ImportType);
}

FText SglTFImporterOptionsWindow::GetImportTypeText(EglTFImportType InImportType) const
{
    FText ImportTypeText(LOCTEXT("None", "None"));
    switch (InImportType)
    {
    case EglTFImportType::StaticMesh:
        ImportTypeText = LOCTEXT("EglTFImportType::StaticMesh", "StaticMesh");
        break;

    case EglTFImportType::SkeletalMesh:
        ImportTypeText = LOCTEXT("EglTFImportType::SkeletalMesh", "SkeletalMesh");
        break;

    case EglTFImportType::Level:
        ImportTypeText = LOCTEXT("EglTFImportType::Level", "Level");
        break;

    default:
        break;
    }
    return ImportTypeText;
}

void SglTFImporterOptionsWindow::HandleMeshScaleRatio(float InNewValue)
{
    glTFImporterOptions.Pin()->MeshScaleRatio = InNewValue;
}

void SglTFImporterOptionsWindow::HandleMeshInvertNormal(ECheckBoxState InCheckBoxState)
{
    glTFImporterOptions.Pin()->bInvertNormal = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImporterOptionsWindow::HandleMeshUseMikkTSpace(ECheckBoxState InCheckBoxState)
{
    glTFImporterOptions.Pin()->bUseMikkTSpace = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImporterOptionsWindow::HandleMeshRecomputeNormals(ECheckBoxState InCheckBoxState)
{
    glTFImporterOptions.Pin()->bRecomputeNormals = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImporterOptionsWindow::HandleMeshRecomputeTangents(ECheckBoxState InCheckBoxState)
{
    glTFImporterOptions.Pin()->bRecomputeTangents = (InCheckBoxState == ECheckBoxState::Checked);
}

bool SglTFImporterOptionsWindow::CanHandleIntegrateAllMeshsForStaticMesh() const
{
    //TSharedPtr<FglTFImporterOptions> glTFImporterOptionsPtr = glTFImporterOptions.Pin();
    //return glTFImporterOptionsPtr->ImportType == EglTFImportType::StaticMesh;
    return false;
}

ECheckBoxState SglTFImporterOptionsWindow::CheckHandleIntegrateAllMeshsForStaticMesh() const
{
    return (glTFImporterOptions.Pin()->bIntegrateAllMeshsForStaticMesh ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void SglTFImporterOptionsWindow::HandleIntegrateAllMeshsForStaticMesh(ECheckBoxState InCheckBoxState)
{
    glTFImporterOptions.Pin()->bIntegrateAllMeshsForStaticMesh = (InCheckBoxState == ECheckBoxState::Checked);
}

bool SglTFImporterOptionsWindow::CanHandleImportAnimationForSkeletalMesh() const
{
    TSharedPtr<FglTFImporterOptions> glTFImporterOptionsPtr = glTFImporterOptions.Pin();
    return (bHasAnimation && glTFImporterOptionsPtr->ImportType == EglTFImportType::SkeletalMesh);
}

ECheckBoxState SglTFImporterOptionsWindow::CheckHandleImportAnimationForSkeleton() const
{
    return (glTFImporterOptions.Pin()->bImportAnimationForSkeletalMesh ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void SglTFImporterOptionsWindow::HandleImportAnimationForSkeletalMesh(ECheckBoxState InCheckBoxState)
{
    glTFImporterOptions.Pin()->bImportAnimationForSkeletalMesh = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImporterOptionsWindow::HandleImportMaterial(ECheckBoxState InCheckBoxState)
{
    glTFImporterOptions.Pin()->bImportMaterial = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImporterOptionsWindow::HandleImportTexture(ECheckBoxState InCheckBoxState)
{
    glTFImporterOptions.Pin()->bImportTexture = (InCheckBoxState == ECheckBoxState::Checked);
}

bool SglTFImporterOptionsWindow::HasAnimation() const
{
    return bHasAnimation;
}

#undef LOCTEXT_NAMESPACE
