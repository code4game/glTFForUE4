#pragma once

#include "SlateBasics.h"
#include "AssetRegistryModule.h"
#include "glTF/glTFImportWindowUI.h"

class SglTFImportWindow : public SCompoundWidget
{
public:
    static TSharedPtr<struct FglTFImportOptions> Open();

public:
    SLATE_BEGIN_ARGS(SglTFImportWindow)
        : _WidgetWindow()
        , _glTFImportWindowUI(nullptr)
        {}

        SLATE_ARGUMENT(UglTFImportWindowUI*, glTFImportWindowUI)
        SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
    SLATE_END_ARGS()

public:
    SglTFImportWindow();

public:
    void Construct(const FArguments& InArgs);

public:
    virtual bool SupportsKeyboardFocus() const override;
    virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

protected:
    bool CanImport() const;
    FReply OnImport();
    FReply OnCancel();

private:
    TWeakPtr<SWindow> WidgetWindow;
    UglTFImportWindowUI* glTFImportWindowUI;
};
