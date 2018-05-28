// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4PrivatePCH.h"
#include "glTF/glTFImportOptionsWindow.h"

#include "glTF/glTFImportOptions.h"

#include "libgltf/libgltf.h"

#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "FglTFForUE4"

TSharedPtr<FglTFImportOptions> SglTFImportOptionsWindow::Open(const FString& InFilePathInOS, const FString& InFilePathInEngine, const libgltf::SGlTF& InGlTF, bool& OutCancel)
{
    TSharedPtr<FglTFImportOptions> glTFImportOptions = MakeShareable(new FglTFImportOptions());
    (*glTFImportOptions) = FglTFImportOptions::Current;

    glTFImportOptions->FilePathInOS = InFilePathInOS;
    glTFImportOptions->FilePathInEngine = InFilePathInEngine;

    /*TSharedPtr<SWindow> ParentWindow;

    //TODO: get the parent window

    TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(LOCTEXT("glTFImportWindowTitle", "Import glTF"))
        .SizingRule(ESizingRule::Autosized);

    TSharedPtr<SglTFImportOptionsWindow> glTFImportWindow;
    Window->SetContent
    (
        SAssignNew(glTFImportWindow, SglTFImportOptionsWindow)
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
    }*/
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
    glTFImportOptions = InArgs._glTFImportOptions;
    checkf(glTFImportOptions.IsValid(), TEXT("Why the argument - glTFImportOptions is null?"));
    WidgetWindow = InArgs._WidgetWindow;

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

void SglTFImportOptionsWindow::HandleImportType(const TSharedPtr<EglTFImportType> InImportType, ESelectInfo::Type InSelectInfo)
{
    if (!InImportType.IsValid()) return;
    glTFImportOptions.Pin()->ImportType = *InImportType;
}

TSharedRef<SWidget> SglTFImportOptionsWindow::GenerateImportType(TSharedPtr<EglTFImportType> InImportType) const
{
    FText ImportTypeText = GetImportTypeText(InImportType.IsValid() ? *InImportType : EglTFImportType::None);
    return SNew(STextBlock)
        .Text(ImportTypeText);
}

FText SglTFImportOptionsWindow::GetImportTypeText() const
{
    return GetImportTypeText(glTFImportOptions.Pin()->ImportType);
}

FText SglTFImportOptionsWindow::GetImportTypeText(EglTFImportType InImportType) const
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

    case EglTFImportType::Actor:
        ImportTypeText = LOCTEXT("EglTFImportType::Actor", "Actor");
        break;

    case EglTFImportType::Level:
        ImportTypeText = LOCTEXT("EglTFImportType::Level", "Level");
        break;

    default:
        break;
    }
    return ImportTypeText;
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

bool SglTFImportOptionsWindow::CanHandleIntegrateAllMeshsForStaticMesh() const
{
    //TSharedPtr<FglTFImportOptions> glTFImportOptionsPtr = glTFImportOptions.Pin();
    //return glTFImportOptionsPtr->ImportType == EglTFImportType::StaticMesh;
    return false;
}

ECheckBoxState SglTFImportOptionsWindow::CheckHandleIntegrateAllMeshsForStaticMesh() const
{
    return (glTFImportOptions.Pin()->bIntegrateAllMeshsForStaticMesh ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void SglTFImportOptionsWindow::HandleIntegrateAllMeshsForStaticMesh(ECheckBoxState InCheckBoxState)
{
    glTFImportOptions.Pin()->bIntegrateAllMeshsForStaticMesh = (InCheckBoxState == ECheckBoxState::Checked);
}

bool SglTFImportOptionsWindow::CanHandleImportAnimationForSkeletalMesh() const
{
    TSharedPtr<FglTFImportOptions> glTFImportOptionsPtr = glTFImportOptions.Pin();
    return (bHasAnimation && glTFImportOptionsPtr->ImportType == EglTFImportType::SkeletalMesh);
}

ECheckBoxState SglTFImportOptionsWindow::CheckHandleImportAnimationForSkeleton() const
{
    return (glTFImportOptions.Pin()->bImportAnimationForSkeletalMesh ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void SglTFImportOptionsWindow::HandleImportAnimationForSkeletalMesh(ECheckBoxState InCheckBoxState)
{
    glTFImportOptions.Pin()->bImportAnimationForSkeletalMesh = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImportOptionsWindow::HandleImportMaterial(ECheckBoxState InCheckBoxState)
{
    glTFImportOptions.Pin()->bImportMaterial = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImportOptionsWindow::HandleImportTexture(ECheckBoxState InCheckBoxState)
{
    glTFImportOptions.Pin()->bImportTexture = (InCheckBoxState == ECheckBoxState::Checked);
}

bool SglTFImportOptionsWindow::HasAnimation() const
{
    return bHasAnimation;
}

#undef LOCTEXT_NAMESPACE
