#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImportWindow.h"

#include "glTF/glTFImportOptions.h"

#include "MainFrame.h"
#include "IDocumentation.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorStyle.h"

#define LOCTEXT_NAMESPACE "FglTFForUE4EdModule"

TSharedPtr<FglTFImportOptions> SglTFImportWindow::Open(const FString& InCurrentFile)
{
    TSharedPtr<FglTFImportOptions> glTFImportOptions = MakeShareable(new FglTFImportOptions());

    TSharedPtr<SWindow> ParentWindow;

    if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
    {
        IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
        ParentWindow = MainFrame.GetParentWindow();
    }

    TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(LOCTEXT("glTFImportWindowTitle", "Import glTF"))
        .SizingRule(ESizingRule::Autosized);

    TSharedPtr<SglTFImportWindow> glTFImportWindow;
    Window->SetContent
    (
        SAssignNew(glTFImportWindow, SglTFImportWindow)
            .glTFImportOptions(glTFImportOptions)
            .WidgetWindow(Window)
            .CurrentFile(FText::FromString(InCurrentFile))
    );

    FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

    //
    return glTFImportOptions;
}

SglTFImportWindow::SglTFImportWindow()
    : SCompoundWidget()
    , glTFImportOptions(nullptr)
    , WidgetWindow(nullptr)
{
    //
}

void SglTFImportWindow::Construct(const FArguments& InArgs)
{
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
                .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                    .AutoWidth()
                [
                    SNew(STextBlock)
                        .Font(FEditorStyle::GetFontStyle("CurveEd.LabelFont"))
                        .Text(LOCTEXT("SglTFImportWindow_CurrentFileTitle", "Current File: "))
                ]
                + SHorizontalBox::Slot()
                    .Padding(5, 0, 0, 0)
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                    .Text(InArgs._CurrentFile)
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
                .HAlign(HAlign_Center)
                .Text(LOCTEXT("SglTFImportWindow_OnImport", "Import file"))
                .ToolTipText(LOCTEXT("SglTFImportWindow_OnImport_ToolTip", "Import file"))
                .IsEnabled(this, &SglTFImportWindow::CanImport)
                .OnClicked(this, &SglTFImportWindow::OnImport)
            ]
            + SUniformGridPanel::Slot(2, 0)
            [
                SNew(SButton)
                    .HAlign(HAlign_Center)
                    .Text(LOCTEXT("SglTFImportWindow_OnCancel", "Cancel"))
                    .ToolTipText(LOCTEXT("SglTFImportWindow_OnCancel_ToolTip", "Cancels importing this glTF file"))
                    .OnClicked(this, &SglTFImportWindow::OnCancel)
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
                [
                    SNew(STextBlock)
                        .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                        .Text(LOCTEXT("SglTFImportWindow_Mesh_Title", "Mesh"))
                ]
                + SVerticalBox::Slot()
                    .AutoHeight()
                [
                    SNew(SBorder)
                        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
                    [
                        SNew(SGridPanel)
                        + SGridPanel::Slot(0, 0)
                            .Padding(2)
                        [
                            SNew(STextBlock)
                                .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("SglTFImportWindow_InvertNormal_Title", "Invert Normal:"))
                        ]
                        + SGridPanel::Slot(1, 0)
                            .Padding(2)
                        [
                            SNew(SCheckBox)
                                .IsChecked(glTFImportOptions.Pin()->bInvertNormal ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImportWindow::HandleMeshInvertNormal)
                        ]
                        + SGridPanel::Slot(0, 1)
                            .Padding(2)
                        [
                            SNew(STextBlock)
                                .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("SglTFImportWindow_InvertTangent_Title", "Scale Ratio:"))
                        ]
                        + SGridPanel::Slot(1, 1)
                            .Padding(2)
                        [
                            SNew(SSpinBox<float>)
                                .Value(glTFImportOptions.Pin()->MeshScaleRatio)
                                .MinValue(0.0f)
                                .MaxValue(1000.0f)
                                .OnValueChanged(this, &SglTFImportWindow::HandleMeshScaleRatio)
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
                [
                    SNew(STextBlock)
                        .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                        .Text(LOCTEXT("SglTFImportWindow_Material_Title", "Material"))
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
                        [
                            SNew(STextBlock)
                                .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
                                .Text(LOCTEXT("SglTFImportWindow_ImportMaterial_Title", "Import Material?"))
                        ]
                        + SGridPanel::Slot(1, 0)
                            .Padding(2)
                        [
                            SNew(SCheckBox)
                                .IsChecked(glTFImportOptions.Pin()->bImportMaterial ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SglTFImportWindow::HandleMaterialImportMaterial)
                        ]
                    ]
                ]
            ]
        ]
    );
}

bool SglTFImportWindow::SupportsKeyboardFocus() const
{
    return true;
}

FReply SglTFImportWindow::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        return OnCancel();
    }

    return FReply::Unhandled();
}

bool SglTFImportWindow::CanImport() const
{
    //
    return false;
}

FReply SglTFImportWindow::OnImport()
{
    //

    if (WidgetWindow.IsValid())
    {
        WidgetWindow.Pin()->RequestDestroyWindow();
    }
    return FReply::Unhandled();
}

FReply SglTFImportWindow::OnCancel()
{
    if (WidgetWindow.IsValid())
    {
        WidgetWindow.Pin()->RequestDestroyWindow();
    }
    return FReply::Handled();
}

void SglTFImportWindow::HandleMeshScaleRatio(float InNewValue)
{
    check(glTFImportOptions.IsValid());

    glTFImportOptions.Pin()->MeshScaleRatio = InNewValue;
}

void SglTFImportWindow::HandleMeshInvertNormal(ECheckBoxState InCheckBoxState)
{
    check(glTFImportOptions.IsValid());

    glTFImportOptions.Pin()->bInvertNormal = (InCheckBoxState == ECheckBoxState::Checked);
}

void SglTFImportWindow::HandleMaterialImportMaterial(ECheckBoxState InCheckBoxState)
{
    check(glTFImportOptions.IsValid());

    glTFImportOptions.Pin()->bImportMaterial = (InCheckBoxState == ECheckBoxState::Checked);
}

#undef LOCTEXT_NAMESPACE
