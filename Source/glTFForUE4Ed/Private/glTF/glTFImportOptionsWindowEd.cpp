// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImportOptionsWindowEd.h"

#include "glTF/glTFImportOptions.h"
#include "glTF/glTFImporterEd.h"

//#include "SlateBasics.h"
#include "Interfaces/IMainFrameModule.h"
#include "IDocumentation.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorStyle.h"

#define LOCTEXT_NAMESPACE "glTFForUE4EdModule"

TSharedPtr<FglTFImportOptions> SglTFImportOptionsWindowEd::Open(const FString& InFilePathInOS, const FString& InFilePathInEngine, const libgltf::SGlTF& InGlTF, bool& OutCancel)
{
    TSharedPtr<FglTFImportOptions> glTFImportOptions = MakeShareable(new FglTFImportOptions());
    (*glTFImportOptions) = FglTFImportOptions::Current;

    glTFImportOptions->FilePathInOS = InFilePathInOS;
    glTFImportOptions->FilePathInEngine = InFilePathInEngine;

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
    else if (glTFImportOptions->ImportType == EglTFImportType::SkeletalMesh)
    {
        glTFImportOptions->ImportType = EglTFImportType::StaticMesh;
    }
    //ImportTypes.Add(TSharedPtr<EglTFImportType>(new EglTFImportType(EglTFImportType::Actor)));
    //ImportTypes.Add(TSharedPtr<EglTFImportType>(new EglTFImportType(EglTFImportType::Level)));

    TSharedPtr<SglTFImportOptionsWindowEd> glTFImportWindow;
    Window->SetContent
    (
        SAssignNew(glTFImportWindow, SglTFImportOptionsWindowEd)
            .glTFImportOptions(glTFImportOptions)
            .WidgetWindow(Window)
            .ImportTypes(ImportTypes)
            .bHasAnimation(InGlTF.animations.size() > 0)
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

SglTFImportOptionsWindowEd::SglTFImportOptionsWindowEd()
    : Super()
{
    //
}

void SglTFImportOptionsWindowEd::Construct(const FArguments& InArgs)
{
    glTFImportOptions = InArgs._glTFImportOptions;
    checkf(glTFImportOptions.IsValid(), TEXT("Why the argument - glTFImportOptions is null?"));
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
                        .Text(FText::FromString(glTFImportOptions.Pin()->FilePathInOS))
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
                IDocumentation::Get()->CreateAnchor(FString("https://doc.c4g.io/gltfforue4/"))
            ]
            + SUniformGridPanel::Slot(1, 0)
            [
                SNew(SButton)
                    .ToolTipText(LOCTEXT("ImportOptionsWindow_OnImport_ToolTip", "Import this glTF file"))
                    .HAlign(HAlign_Center)
                    .Text(LOCTEXT("ImportOptionsWindow_OnImport", "Import file"))
                    .IsEnabled(this, &SglTFImportOptionsWindowEd::CanImport)
                    .OnClicked(this, &SglTFImportOptionsWindowEd::OnImport)
            ]
            + SUniformGridPanel::Slot(2, 0)
            [
                SNew(SButton)
                    .ToolTipText(LOCTEXT("ImportOptionsWindow_OnCancel_ToolTip", "Cancel to import this glTF file"))
                    .HAlign(HAlign_Center)
                    .Text(LOCTEXT("ImportOptionsWindow_OnCancel", "Cancel"))
                    .OnClicked(this, &SglTFImportOptionsWindowEd::OnCancel)
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
                                .OnSelectionChanged(this, &SglTFImportOptionsWindowEd::HandleImportType)
                                .OnGenerateWidget(this, &SglTFImportOptionsWindowEd::GenerateImportType)
                                .Content()
                                [
                                    SNew(STextBlock)
                                        .Text(this, &SglTFImportOptionsWindowEd::GetImportTypeText)
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
                                .Value(glTFImportOptions.Pin()->MeshScaleRatio)
                                .MinValue(0.0f)
                                .MaxValue(100000.0f)
                                .OnValueChanged(this, &SglTFImportOptionsWindowEd::HandleMeshScaleRatio)
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
                                .IsChecked(glTFImportOptions.Pin()->bInvertNormal ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImportOptionsWindowEd::HandleMeshInvertNormal)
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
                                .IsChecked(glTFImportOptions.Pin()->bUseMikkTSpace ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImportOptionsWindowEd::HandleMeshUseMikkTSpace)
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
                                .IsChecked(glTFImportOptions.Pin()->bRecomputeNormals ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImportOptionsWindowEd::HandleMeshRecomputeNormals)
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
                                .IsChecked(glTFImportOptions.Pin()->bRecomputeTangents ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImportOptionsWindowEd::HandleMeshRecomputeTangents)
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
                            .IsEnabled(this, &SglTFImportOptionsWindowEd::CanHandleIntegrateAllMeshsForStaticMesh)
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
                                .IsChecked(this, &SglTFImportOptionsWindowEd::CheckHandleIntegrateAllMeshsForStaticMesh)
                                .OnCheckStateChanged(this, &SglTFImportOptionsWindowEd::HandleIntegrateAllMeshsForStaticMesh)
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
                            .IsEnabled(this, &SglTFImportOptionsWindowEd::CanHandleImportAnimationForSkeletalMesh)
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
                                .IsChecked(this, &SglTFImportOptionsWindowEd::CheckHandleImportAnimationForSkeleton)
                                .OnCheckStateChanged(this, &SglTFImportOptionsWindowEd::HandleImportAnimationForSkeletalMesh)
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
                                .IsChecked(glTFImportOptions.Pin()->bImportMaterial ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImportOptionsWindowEd::HandleImportMaterial)
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
                                .IsChecked(glTFImportOptions.Pin()->bImportTexture ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImportOptionsWindowEd::HandleImportTexture)
                        ]
                    ]
                ]
            ]
        ]
    );
}

#undef LOCTEXT_NAMESPACE
