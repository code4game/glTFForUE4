#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImportWindow.h"

#include "glTF/glTFImportWindowUI.h"
#include "glTF/glTFImportOptions.h"

#include "MainFrame.h"
#include "IDocumentation.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorStyle.h"

#define LOCTEXT_NAMESPACE "FglTFForUE4EdModule"

TSharedPtr<FglTFImportOptions> SglTFImportWindow::Open()
{
    TSharedPtr<FglTFImportOptions> glTFImportOptions(nullptr);

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
            .glTFImportWindowUI(nullptr)
            .WidgetWindow(Window)
    );

    FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

    //
    return glTFImportOptions;
}

SglTFImportWindow::SglTFImportWindow()
    : SCompoundWidget()
    , glTFImportWindowUI(nullptr)
    , WidgetWindow(nullptr)
{
    //
}

void SglTFImportWindow::Construct(const FArguments& InArgs)
{
    glTFImportWindowUI = InArgs._glTFImportWindowUI;
    WidgetWindow = InArgs._WidgetWindow;

    TSharedPtr<SBox> InspectorBox;
    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2)
            [
                SNew(SBorder)
                .Padding(FMargin(3))
                .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2)
            [
                SAssignNew(InspectorBox, SBox)
                .MaxDesiredHeight(650.0f)
                .WidthOverride(400.0f)
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

    FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.bAllowSearch = false;
    DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    TSharedPtr<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

    InspectorBox->SetContent(DetailsView->AsShared());
    DetailsView->SetObject(glTFImportWindowUI);
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

#undef LOCTEXT_NAMESPACE
