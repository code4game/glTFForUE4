// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterOptionsWindowEd.h"

#include "glTF/glTFImporterOptions.h"
#include "glTF/glTFImporterEd.h"

//#include "SlateBasics.h"
#include "Interfaces/IMainFrameModule.h"
#include "IDocumentation.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorStyle.h"

#define LOCTEXT_NAMESPACE "glTFForUE4EdModule"

TSharedPtr<FglTFImporterOptions> SglTFImporterOptionsWindowEd::Open(const FString& InFilePathInOS, const FString& InFilePathInEngine, const libgltf::SGlTF& InGlTF, bool& OutCancel)
{
    TSharedPtr<FglTFImporterOptions> glTFImporterOptions = MakeShareable(new FglTFImporterOptions());
    (*glTFImporterOptions) = FglTFImporterOptions::Current;

    glTFImporterOptions->FilePathInOS = InFilePathInOS;
    glTFImporterOptions->FilePathInEngine = InFilePathInEngine;

    TSharedPtr<SWindow> ParentWindow;

    if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
    {
        IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
        ParentWindow = MainFrame.GetParentWindow();
    }

    TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(LOCTEXT("glTFImportWindowTitle", "Import glTF"))
        .SizingRule(ESizingRule::Autosized);

    TArray<TSharedPtr<EglTFImportType>> ImportTypes;
    ImportTypes.Add(TSharedPtr<EglTFImportType>(new EglTFImportType(EglTFImportType::StaticMesh)));
    if (InGlTF.skins.size() > 0)
    {
        ImportTypes.Add(TSharedPtr<EglTFImportType>(new EglTFImportType(EglTFImportType::SkeletalMesh)));
    }
    else if (glTFImporterOptions->ImportType == EglTFImportType::SkeletalMesh)
    {
        glTFImporterOptions->ImportType = EglTFImportType::StaticMesh;
    }
    //ImportTypes.Add(TSharedPtr<EglTFImportType>(new EglTFImportType(EglTFImportType::Actor)));
    //ImportTypes.Add(TSharedPtr<EglTFImportType>(new EglTFImportType(EglTFImportType::Level)));

    TSharedPtr<SglTFImporterOptionsWindowEd> glTFImportWindow;
    Window->SetContent
    (
        SAssignNew(glTFImportWindow, SglTFImporterOptionsWindowEd)
            .glTFImporterOptions(glTFImporterOptions)
            .WidgetWindow(Window)
            .ImportTypes(ImportTypes)
            .bHasAnimation(InGlTF.animations.size() > 0)
    );

    /// Show the import options window.
    FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

    OutCancel = (glTFImporterOptions != glTFImportWindow->GetImportOptions());

    /// Store the option if not cancel
    if (!OutCancel)
    {
        FglTFImporterOptions::Current = (*glTFImporterOptions);
    }
    return glTFImporterOptions;
}

SglTFImporterOptionsWindowEd::SglTFImporterOptionsWindowEd()
    : Super()
{
    //
}

void SglTFImporterOptionsWindowEd::Construct(const FArguments& InArgs)
{
    glTFImporterOptions = InArgs._glTFImporterOptions;
    checkf(glTFImporterOptions.IsValid(), TEXT("Why the argument - glTFImporterOptions is null?"));
    WidgetWindow = InArgs._WidgetWindow;
    ImportTypes = InArgs._ImportTypes;
    bHasAnimation = InArgs._bHasAnimation;

    if (ImportTypes.Num() <= 0)
    {
        ImportTypes.Add(TSharedPtr<EglTFImportType>(new EglTFImportType(EglTFImportType::None)));
    }

    TSharedPtr<SBox> InspectorBox;
    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
            .AutoHeight()
        [
            SNew(SBorder)
                .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
            [
                SNew(SGridPanel)
                + SGridPanel::Slot(0, 0)
                    .Padding(2)
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                        .Font(FEditorStyle::GetFontStyle("CurveEd.LabelFont"))
                        .Text(LOCTEXT("ImportOptionsWindow_SourceFilePath_Title", "Source File Path: "))
                ]
                + SGridPanel::Slot(1, 0)
                    .Padding(2)
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                        .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                        .Text(FText::FromString(glTFImporterOptions.Pin()->FilePathInOS))
                ]
                + SGridPanel::Slot(0, 1)
                    .Padding(2)
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                        .Font(FEditorStyle::GetFontStyle("CurveEd.LabelFont"))
                        .Text(LOCTEXT("ImportOptionsWindow_TargetFilePath_Title", "Target File Path: "))
                ]
                + SGridPanel::Slot(1, 1)
                    .Padding(2)
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                    .Text(FText::FromString(glTFImporterOptions.Pin()->FilePathInEngine))
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
                IDocumentation::Get()->CreateAnchor(FString("https://doc.c4g.io/gltfforue4/"))
            ]
            + SUniformGridPanel::Slot(1, 0)
            [
                SNew(SButton)
                    .ToolTipText(LOCTEXT("ImportOptionsWindow_OnImport_ToolTip", "Import this glTF file"))
                    .HAlign(HAlign_Center)
                    .Text(LOCTEXT("ImportOptionsWindow_OnImport", "Import file"))
                    .IsEnabled(this, &SglTFImporterOptionsWindowEd::CanImport)
                    .OnClicked(this, &SglTFImporterOptionsWindowEd::OnImport)
            ]
            + SUniformGridPanel::Slot(2, 0)
            [
                SNew(SButton)
                    .ToolTipText(LOCTEXT("ImportOptionsWindow_OnCancel_ToolTip", "Cancel to import this glTF file"))
                    .HAlign(HAlign_Center)
                    .Text(LOCTEXT("ImportOptionsWindow_OnCancel", "Cancel"))
                    .OnClicked(this, &SglTFImporterOptionsWindowEd::OnCancel)
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
                .BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(2)
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                        .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                        .Text(LOCTEXT("ImportOptionsWindow_ImportSettings_Title", "Import Settings"))
                ]
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(2)
                [
                    SNew(SBorder)
                        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
                    [
                        SNew(SGridPanel)
                        + SGridPanel::Slot(0, 0)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .ToolTipText(LOCTEXT("ImportOptionsWindow_ImportType_ToolTip", "Import Type!"))
                                .MinDesiredWidth(200)
                                .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("ImportOptionsWindow_ImportType_Title", "Import Type"))
                        ]
                        + SGridPanel::Slot(1, 0)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SComboBox<TSharedPtr<EglTFImportType>>)
                                .InitiallySelectedItem(ImportTypes[0])
                                .OptionsSource(&ImportTypes)
                                .OnSelectionChanged(this, &SglTFImporterOptionsWindowEd::HandleImportType)
                                .OnGenerateWidget(this, &SglTFImporterOptionsWindowEd::GenerateImportType)
                                .Content()
                                [
                                    SNew(STextBlock)
                                        .Text(this, &SglTFImporterOptionsWindowEd::GetImportTypeText)
                                ]
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
                .BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(2)
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                        .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                        .Text(LOCTEXT("ImportOptionsWindow_Mesh_Title", "Mesh"))
                ]
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(2)
                [
                    SNew(SBorder)
                        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
                    [
                        SNew(SGridPanel)
                        + SGridPanel::Slot(0, 0)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("ImportOptionsWindow_MeshScaleRatio_Title", "Scale Ratio"))
                        ]
                        + SGridPanel::Slot(1, 0)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SSpinBox<float>)
                                .Value(glTFImporterOptions.Pin()->MeshScaleRatio)
                                .MinValue(0.0f)
                                .MaxValue(100000.0f)
                                .OnValueChanged(this, &SglTFImporterOptionsWindowEd::HandleMeshScaleRatio)
                        ]
                        + SGridPanel::Slot(0, 1)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .MinDesiredWidth(200)
                                .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("ImportOptionsWindow_InvertNormal_Title", "Invert Normal: "))
                        ]
                        + SGridPanel::Slot(1, 1)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsChecked(glTFImporterOptions.Pin()->bInvertNormal ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImporterOptionsWindowEd::HandleMeshInvertNormal)
                        ]
                        + SGridPanel::Slot(0, 2)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .MinDesiredWidth(200)
                                .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("ImportOptionsWindow_UseMikkTSpace_Title", "Use MikkT Space: "))
                        ]
                        + SGridPanel::Slot(1, 2)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsChecked(glTFImporterOptions.Pin()->bUseMikkTSpace ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImporterOptionsWindowEd::HandleMeshUseMikkTSpace)
                        ]
                        + SGridPanel::Slot(0, 3)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .MinDesiredWidth(200)
                                .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("ImportOptionsWindow_RecomputeNormals_Title", "Recompute Normals: "))
                        ]
                        + SGridPanel::Slot(1, 3)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsChecked(glTFImporterOptions.Pin()->bRecomputeNormals ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImporterOptionsWindowEd::HandleMeshRecomputeNormals)
                        ]
                        + SGridPanel::Slot(0, 4)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .MinDesiredWidth(200)
                                .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("ImportOptionsWindow_RecomputeTangents_Title", "Recompute Tangents: "))
                        ]
                        + SGridPanel::Slot(1, 4)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsChecked(glTFImporterOptions.Pin()->bRecomputeTangents ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImporterOptionsWindowEd::HandleMeshRecomputeTangents)
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
                .BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(2)
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                        .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                        .Text(LOCTEXT("ImportOptionsWindow_StaticMesh_Title", "Static Mesh"))
                ]
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(2)
                [
                    SNew(SBorder)
                        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
                    [
                        SNew(SGridPanel)
                            .IsEnabled(this, &SglTFImporterOptionsWindowEd::CanHandleIntegrateAllMeshsForStaticMesh)
                        + SGridPanel::Slot(0, 0)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .ToolTipText(LOCTEXT("ImportOptionsWindow_IntegrateAllMeshsForStaticMesh_ToolTip", "Integrate All Meshs!"))
                                .MinDesiredWidth(200)
                                .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("ImportOptionsWindow_IntegrateAllMeshsForStaticMesh_Title", "Integrate All Meshs: "))
                        ]
                        + SGridPanel::Slot(1, 0)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsChecked(this, &SglTFImporterOptionsWindowEd::CheckHandleIntegrateAllMeshsForStaticMesh)
                                .OnCheckStateChanged(this, &SglTFImporterOptionsWindowEd::HandleIntegrateAllMeshsForStaticMesh)
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
                .BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(2)
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                        .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                        .Text(LOCTEXT("ImportOptionsWindow_SkeletonMesh_Title", "Skeleton Mesh"))
                ]
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(2)
                [
                    SNew(SBorder)
                        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
                    [
                        SNew(SGridPanel)
                            .IsEnabled(this, &SglTFImporterOptionsWindowEd::CanHandleImportAnimationForSkeletalMesh)
                        + SGridPanel::Slot(0, 0)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .ToolTipText(LOCTEXT("ImportOptionsWindow_ImportAnimationForSkeletonMesh_ToolTip", "Import Animation!"))
                                .MinDesiredWidth(200)
                                .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("ImportOptionsWindow_ImportAnimationForSkeletonMesh_Title", "Import Animation: "))
                        ]
                        + SGridPanel::Slot(1, 0)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsChecked(this, &SglTFImporterOptionsWindowEd::CheckHandleImportAnimationForSkeleton)
                                .OnCheckStateChanged(this, &SglTFImporterOptionsWindowEd::HandleImportAnimationForSkeletalMesh)
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
                .BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(2)
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                        .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                        .Text(LOCTEXT("ImportOptionsWindow_Material_Title", "Material"))
                ]
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(2)
                [
                    SNew(SBorder)
                        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
                    [
                        SNew(SGridPanel)
                        + SGridPanel::Slot(0, 0)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .ToolTipText(LOCTEXT("ImportOptionsWindow_ImportMaterial_ToolTip", "Import Material!"))
                                .MinDesiredWidth(200)
                                .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("ImportOptionsWindow_ImportMaterial_Title", "Import Material: "))
                        ]
                        + SGridPanel::Slot(1, 0)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsChecked(glTFImporterOptions.Pin()->bImportMaterial ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImporterOptionsWindowEd::HandleImportMaterial)
                        ]
                        + SGridPanel::Slot(0, 1)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .ToolTipText(LOCTEXT("ImportOptionsWindow_ImportTexture_ToolTip", "Import Texture!"))
                                .MinDesiredWidth(200)
                                .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("ImportOptionsWindow_ImportTexture_Title", "Import Texture"))
                        ]
                        + SGridPanel::Slot(1, 1)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsChecked(glTFImporterOptions.Pin()->bImportTexture ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImporterOptionsWindowEd::HandleImportTexture)
                        ]
                    ]
                ]
            ]
        ]
    );
}

#undef LOCTEXT_NAMESPACE
