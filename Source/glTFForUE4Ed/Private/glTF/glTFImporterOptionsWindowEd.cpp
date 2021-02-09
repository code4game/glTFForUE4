// Copyright(c) 2016 - 2021 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterOptionsWindowEd.h"

#include "glTF/glTFImporterOptions.h"
#include "glTF/glTFImporterEd.h"

#include <SlateBasics.h>
#include <Interfaces/IMainFrameModule.h>
#include <IDocumentation.h>
#include <PropertyEditorModule.h>
#include <IDetailsView.h>

#if (ENGINE_MINOR_VERSION <= 25)
#else
#include <Misc/SecureHash.h>
#endif

#define LOCTEXT_NAMESPACE "glTFForUE4EdModule"

TSharedPtr<FglTFImporterOptions> SglTFImporterOptionsWindowEd::Open(UObject* InContext, const FString& InFilePathInOS, const FString& InFilePathInEngine, const libgltf::SGlTF& InGlTF, bool& OutCancel)
{
    TSharedPtr<FglTFImporterOptions> glTFImporterOptions = MakeShareable(new FglTFImporterOptions());

    glTFImporterOptions->FilePathInOS = InFilePathInOS;
    glTFImporterOptions->FilePathInEngine = InFilePathInEngine;
    glTFImporterOptions->Details = GetMutableDefault<UglTFImporterOptionsDetails>();
#if (ENGINE_MINOR_VERSION <= 13)
#else
    glTFImporterOptions->FileHash = MakeShared<FMD5Hash>();
    *glTFImporterOptions->FileHash = FMD5Hash::HashFile(*glTFImporterOptions->FilePathInOS);
#endif

    TSharedPtr<SWindow> ParentWindow;

    if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
    {
        IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
        ParentWindow = MainFrame.GetParentWindow();
    }

    TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(LOCTEXT("glTFImportWindowTitle", "Import glTF"))
        .SizingRule(ESizingRule::Autosized);

    TSharedPtr<SglTFImporterOptionsWindowEd> glTFImportWindow;
    Window->SetContent
    (
        SAssignNew(glTFImportWindow, SglTFImporterOptionsWindowEd)
            .glTFImporterOptions(glTFImporterOptions)
            .WidgetWindow(Window)
    );

    /// Show the import options window.
    FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

    OutCancel = (glTFImporterOptions != glTFImportWindow->GetImportOptions());
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
    TSharedPtr<FglTFImporterOptions> glTFImporterOptionsPtr = glTFImporterOptions.Pin();
    checkf(glTFImporterOptionsPtr->Details, TEXT("Why the argument - glTFImporterOptions->Detail is null?"));
    WidgetWindow = InArgs._WidgetWindow;

    TSharedPtr<SBox> DetailBox;
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
                        .Text(FText::FromString(glTFImporterOptionsPtr->FilePathInOS))
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
                    .Text(FText::FromString(glTFImporterOptionsPtr->FilePathInEngine))
                ]
            ]
        ]
        + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 2, 0, 2)
        [
            SAssignNew(DetailBox, SBox)
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

    {
        FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
        FDetailsViewArgs DetailsViewArgs;
        DetailsViewArgs.bAllowSearch = false;
        DetailsViewArgs.bAllowMultipleTopLevelObjects = true;
        DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
        TSharedPtr<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
        DetailsView->SetObject(glTFImporterOptionsPtr->Details);
        DetailBox->SetContent(DetailsView.ToSharedRef());
    }
}

#undef LOCTEXT_NAMESPACE
