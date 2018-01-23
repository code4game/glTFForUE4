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

    TSharedPtr<SBox> InspectorBox;
    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
            .AutoHeight()
        [
            SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
            [
                SNew(SGridPanel)
                + SGridPanel::Slot(0, 0)
                    .Padding(2)
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::Get().GetFontStyle("CurveEd.LabelFont"))
                        .Text(LOCTEXT("SglTFImportWindow_SourceFilePath_Title", "Source File Path: "))
                ]
                + SGridPanel::Slot(1, 0)
                    .Padding(2)
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::Get().GetFontStyle("CurveEd.InfoFont"))
                        .Text(FText::FromString(glTFImportOptions.Pin()->FilePathInOS))
                ]
                + SGridPanel::Slot(0, 1)
                    .Padding(2)
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::Get().GetFontStyle("CurveEd.LabelFont"))
                        .Text(LOCTEXT("SglTFImportWindow_TargetFilePath_Title", "Target File Path: "))
                ]
                + SGridPanel::Slot(1, 1)
                    .Padding(2)
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Font(FCoreStyle::Get().GetFontStyle("CurveEd.InfoFont"))
                    .Text(FText::FromString(glTFImportOptions.Pin()->FilePathInEngine))
                ]
            ]
        ]
        + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 2, 0, 2)
        [
            SAssignNew(InspectorBox, SBox)
        ]
        + SVerticalBox::Slot()
            .AutoHeight()
            .HAlign(HAlign_Right)
            .Padding(2)
        [
            SNew(SUniformGridPanel)
                .SlotPadding(2)
            + SUniformGridPanel::Slot(0, 0)
            [
                SNew(SButton)
                    .ToolTipText(LOCTEXT("SglTFImportWindow_OnImport_ToolTip", "Import this glTF file"))
                    .HAlign(HAlign_Center)
                    .Text(LOCTEXT("SglTFImportWindow_OnImport", "Import file"))
                    .IsEnabled(this, &SglTFImportOptionsWindow::CanImport)
                    .OnClicked(this, &SglTFImportOptionsWindow::OnImport)
            ]
            + SUniformGridPanel::Slot(1, 0)
            [
                SNew(SButton)
                    .ToolTipText(LOCTEXT("SglTFImportWindow_OnCancel_ToolTip", "Cancel to import this glTF file"))
                    .HAlign(HAlign_Center)
                    .Text(LOCTEXT("SglTFImportWindow_OnCancel", "Cancel"))
                    .OnClicked(this, &SglTFImportOptionsWindow::OnCancel)
            ]
        ]
    ];

    InspectorBox->SetContent(
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2)
        [
            SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(2)
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::Get().GetFontStyle("CurveEd.InfoFont"))
                        .Text(LOCTEXT("SglTFImportWindow_BuildSetting_Title", "Build Setting"))
                ]
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(2)
                [
                    SNew(SBorder)
                        .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                    [
                        SNew(SGridPanel)
                        + SGridPanel::Slot(0, 0)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .ToolTipText(NSLOCTEXT("glTFForUE4Ed", "ImportAllScenes_ToolTip", "Import all scenes!"))
                                .MinDesiredWidth(200)
                                .Font(FCoreStyle::Get().GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("SglTFImportWindow_ImportAllScenes_Title", "Import All Scenes"))
                        ]
                        + SGridPanel::Slot(1, 0)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .ToolTipText(NSLOCTEXT("glTFForUE4Ed", "ImportAllScenes_ToolTip", "Import all scenes!"))
                                .IsChecked(glTFImportOptions.Pin()->bImportAllScenes ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImportOptionsWindow::HandleImportAllScenes)
                        ]
                        + SGridPanel::Slot(0, 1)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .IsEnabled(false)
                                .ToolTipText(NSLOCTEXT("glTFForUE4Ed", "ImportSkeleton_ToolTip", "Import skeleton!"))
                                .MinDesiredWidth(200)
                                .Font(FCoreStyle::Get().GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("SglTFImportWindow_ImportSkeleton_Title", "Import Skeleton"))
                        ]
                        + SGridPanel::Slot(1, 1)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsEnabled(false)
                                .ToolTipText(NSLOCTEXT("glTFForUE4Ed", "ImportSkeleton_ToolTip", "Import skeleton!"))
                                .IsChecked(glTFImportOptions.Pin()->bImportSkeleton ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImportOptionsWindow::HandleImportSkeleton)
                        ]
                        + SGridPanel::Slot(0, 2)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .IsEnabled(false)
                                .ToolTipText(NSLOCTEXT("glTFForUE4Ed", "ImportMaterial_ToolTip", "This function is in developing!"))
                                .MinDesiredWidth(200)
                                .Font(FCoreStyle::Get().GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("SglTFImportWindow_ImportMaterial_Title", "Import Material: "))
                        ]
                        + SGridPanel::Slot(1, 2)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsEnabled(false)
                                .ToolTipText(NSLOCTEXT("glTFForUE4Ed", "ImportMaterial_ToolTip", "This function is in developing!"))
                                .IsChecked(glTFImportOptions.Pin()->bImportMaterial ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImportOptionsWindow::HandleImportMaterial)
                        ]
                    ]
                ]
            ]
        ]
        + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2)
        [
            SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(2)
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::Get().GetFontStyle("CurveEd.InfoFont"))
                        .Text(LOCTEXT("SglTFImportWindow_Mesh_Title", "Mesh"))
                ]
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(2)
                [
                    SNew(SBorder)
                        .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                    [
                        SNew(SGridPanel)
                        + SGridPanel::Slot(0, 0)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .Font(FCoreStyle::Get().GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("SglTFImportWindow_MeshScaleRatio_Title", "Scale Ratio"))
                        ]
                        + SGridPanel::Slot(1, 0)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                                .AutoWidth()
                                .Padding(2)
                                .HAlign(HAlign_Left)
                                .VAlign(VAlign_Center)
                            [
                                SNew(STextBlock)
                                    .Font(FCoreStyle::Get().GetFontStyle("CurveEd.InfoFont"))
                                    .Text(LOCTEXT("SglTFImportWindow_MeshScaleRatioX_Title", "X"))
                            ]
                            + SHorizontalBox::Slot()
                                .AutoWidth()
                                .Padding(2)
                                .HAlign(HAlign_Left)
                                .VAlign(VAlign_Center)
                            [
                                SNew(SSpinBox<float>)
                                    .Value(glTFImportOptions.Pin()->MeshScaleRatio.X)
                                    .MinValue(0.0f)
                                    .MaxValue(100000.0f)
                                    .OnValueChanged(this, &SglTFImportOptionsWindow::HandleMeshScaleRatioX)
                            ]
                            + SHorizontalBox::Slot()
                                .AutoWidth()
                                .Padding(2)
                                .HAlign(HAlign_Left)
                                .VAlign(VAlign_Center)
                            [
                                SNew(STextBlock)
                                    .Font(FCoreStyle::Get().GetFontStyle("CurveEd.InfoFont"))
                                    .Text(LOCTEXT("SglTFImportWindow_MeshScaleRatioY_Title", "Y"))
                            ]
                            + SHorizontalBox::Slot()
                                .AutoWidth()
                                .Padding(2)
                                .HAlign(HAlign_Left)
                                .VAlign(VAlign_Center)
                            [
                                SNew(SSpinBox<float>)
                                    .Value(glTFImportOptions.Pin()->MeshScaleRatio.Y)
                                    .MinValue(0.0f)
                                    .MaxValue(100000.0f)
                                    .OnValueChanged(this, &SglTFImportOptionsWindow::HandleMeshScaleRatioY)
                            ]
                            + SHorizontalBox::Slot()
                                .AutoWidth()
                                .Padding(2)
                                .HAlign(HAlign_Left)
                                .VAlign(VAlign_Center)
                            [
                                SNew(STextBlock)
                                    .Font(FCoreStyle::Get().GetFontStyle("CurveEd.InfoFont"))
                                    .Text(LOCTEXT("SglTFImportWindow_MeshScaleRatioZ_Title", "Z"))
                            ]
                            + SHorizontalBox::Slot()
                                .AutoWidth()
                                .Padding(2)
                                .HAlign(HAlign_Left)
                                .VAlign(VAlign_Center)
                            [
                                SNew(SSpinBox<float>)
                                    .Value(glTFImportOptions.Pin()->MeshScaleRatio.Z)
                                    .MinValue(0.0f)
                                    .MaxValue(100000.0f)
                                    .OnValueChanged(this, &SglTFImportOptionsWindow::HandleMeshScaleRatioZ)
                            ]
                        ]
                        + SGridPanel::Slot(0, 1)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .MinDesiredWidth(200)
                                .Font(FCoreStyle::Get().GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("SglTFImportWindow_InvertNormal_Title", "Invert Normal: "))
                        ]
                        + SGridPanel::Slot(1, 1)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsChecked(glTFImportOptions.Pin()->bInvertNormal ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImportOptionsWindow::HandleMeshInvertNormal)
                        ]
                        + SGridPanel::Slot(0, 2)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .MinDesiredWidth(200)
                                .Font(FCoreStyle::Get().GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("SglTFImportWindow_UseMikkTSpace_Title", "Use MikkT Space: "))
                        ]
                        + SGridPanel::Slot(1, 2)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsChecked(glTFImportOptions.Pin()->bUseMikkTSpace ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImportOptionsWindow::HandleMeshUseMikkTSpace)
                        ]
                        + SGridPanel::Slot(0, 3)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .MinDesiredWidth(200)
                                .Font(FCoreStyle::Get().GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("SglTFImportWindow_RecomputeNormals_Title", "Recompute Normals: "))
                        ]
                        + SGridPanel::Slot(1, 3)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsChecked(glTFImportOptions.Pin()->bRecomputeNormals ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImportOptionsWindow::HandleMeshRecomputeNormals)
                        ]
                        + SGridPanel::Slot(0, 4)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .MinDesiredWidth(200)
                                .Font(FCoreStyle::Get().GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("SglTFImportWindow_RecomputeTangents_Title", "Recompute Tangents: "))
                        ]
                        + SGridPanel::Slot(1, 4)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsChecked(glTFImportOptions.Pin()->bRecomputeTangents ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImportOptionsWindow::HandleMeshRecomputeTangents)
                        ]
                    ]
                ]
            ]
        ]
        + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2)
        [
            SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(2)
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::Get().GetFontStyle("CurveEd.InfoFont"))
                        .Text(LOCTEXT("SglTFImportWindow_Material_Title", "Material"))
                ]
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(2)
                [
                    SNew(SBorder)
                        .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                    [
                        SNew(SGridPanel)
                    ]
                ]
            ]
        ]
    );
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

void SglTFImportOptionsWindow::HandleImportAllScenes(ECheckBoxState InCheckBoxState)
{
    check(glTFImportOptions.IsValid());

    glTFImportOptions.Pin()->bImportAllScenes = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImportOptionsWindow::HandleImportSkeleton(ECheckBoxState InCheckBoxState)
{
    check(glTFImportOptions.IsValid());

    glTFImportOptions.Pin()->bImportSkeleton = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImportOptionsWindow::HandleImportMaterial(ECheckBoxState InCheckBoxState)
{
    check(glTFImportOptions.IsValid());

    glTFImportOptions.Pin()->bImportMaterial = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImportOptionsWindow::HandleMeshScaleRatioX(float InNewValue)
{
    check(glTFImportOptions.IsValid());
    glTFImportOptions.Pin()->MeshScaleRatio.X = InNewValue;
}

void SglTFImportOptionsWindow::HandleMeshScaleRatioY(float InNewValue)
{
    check(glTFImportOptions.IsValid());

    glTFImportOptions.Pin()->MeshScaleRatio.Y = InNewValue;
}

void SglTFImportOptionsWindow::HandleMeshScaleRatioZ(float InNewValue)
{
    check(glTFImportOptions.IsValid());

    glTFImportOptions.Pin()->MeshScaleRatio.Z = InNewValue;
}

void SglTFImportOptionsWindow::HandleMeshInvertNormal(ECheckBoxState InCheckBoxState)
{
    check(glTFImportOptions.IsValid());

    glTFImportOptions.Pin()->bInvertNormal = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImportOptionsWindow::HandleMeshUseMikkTSpace(ECheckBoxState InCheckBoxState)
{
    check(glTFImportOptions.IsValid());

    glTFImportOptions.Pin()->bUseMikkTSpace = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImportOptionsWindow::HandleMeshRecomputeNormals(ECheckBoxState InCheckBoxState)
{
    check(glTFImportOptions.IsValid());

    glTFImportOptions.Pin()->bRecomputeNormals = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImportOptionsWindow::HandleMeshRecomputeTangents(ECheckBoxState InCheckBoxState)
{
    check(glTFImportOptions.IsValid());

    glTFImportOptions.Pin()->bRecomputeTangents = (InCheckBoxState == ECheckBoxState::Checked);
}

#undef LOCTEXT_NAMESPACE
