// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterOptionsWindowEd.h"

#include "glTF/glTFImporterOptions.h"
#include "glTF/glTFImporterEd.h"

#include <SlateBasics.h>
#include <Interfaces/IMainFrameModule.h>
#include <IDocumentation.h>
#include <PropertyEditorModule.h>
#include <IDetailsView.h>

#define LOCTEXT_NAMESPACE "glTFForUE4EdModule"

TSharedPtr<FglTFImporterOptions> SglTFImporterOptionsWindowEd::Open(UObject* InContext, const FString& InFilePathInOS, const FString& InFilePathInEngine, const libgltf::SGlTF& InGlTF, bool& OutCancel)
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
    glTFImporterOptions->ImportType = EglTFImportType::StaticMesh;
    if (InGlTF.skins.size() > 0)
    {
        ImportTypes.Add(TSharedPtr<EglTFImportType>(new EglTFImportType(EglTFImportType::SkeletalMesh)));
        glTFImporterOptions->ImportType = EglTFImportType::SkeletalMesh;
    }
    ImportTypes.Add(TSharedPtr<EglTFImportType>(new EglTFImportType(EglTFImportType::StaticMesh)));
    ImportTypes.Add(TSharedPtr<EglTFImportType>(new EglTFImportType(EglTFImportType::Level)));

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
                        + SGridPanel::Slot(0, 5)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .MinDesiredWidth(200)
                                .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("ImportOptionsWindow_RemoveDegenerates_Title", "Remove Degenerates: "))
                        ]
                        + SGridPanel::Slot(1, 5)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsChecked(glTFImporterOptions.Pin()->bRemoveDegenerates ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImporterOptionsWindowEd::HandleMeshRemoveDegenerates)
                        ]
                        + SGridPanel::Slot(0, 6)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .MinDesiredWidth(200)
                                .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("ImportOptionsWindow_BuildAdjacencyBuffer_Title", "Build Adjacency Buffer: "))
                        ]
                        + SGridPanel::Slot(1, 6)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsChecked(glTFImporterOptions.Pin()->bBuildAdjacencyBuffer ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImporterOptionsWindowEd::HandleMeshBuildAdjacencyBuffer)
                        ]
                        + SGridPanel::Slot(0, 7)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .MinDesiredWidth(200)
                                .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("ImportOptionsWindow_UseFullPrecisionUVs_Title", "Use Full Precision UVs: "))
                        ]
                        + SGridPanel::Slot(1, 7)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsChecked(glTFImporterOptions.Pin()->bUseFullPrecisionUVs ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImporterOptionsWindowEd::HandleMeshUseFullPrecisionUVs)
                        ]
                        + SGridPanel::Slot(0, 8)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .MinDesiredWidth(200)
                                .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("ImportOptionsWindow_GenerateLightmapUVs_Title", "Generate Lightmap UVs: "))
                        ]
                        + SGridPanel::Slot(1, 8)
                            .Padding(2)
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                                .IsChecked(glTFImporterOptions.Pin()->bGenerateLightmapUVs ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImporterOptionsWindowEd::HandleMeshGenerateLightmapUVs)
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
