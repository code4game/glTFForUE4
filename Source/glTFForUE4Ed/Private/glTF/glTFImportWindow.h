#pragma once

#include "SlateBasics.h"
#include "AssetRegistryModule.h"

class SglTFImportWindow : public SCompoundWidget
{
public:
    static TSharedPtr<struct FglTFImportOptions> Open(const FString& InCurrentFile);

public:
    SLATE_BEGIN_ARGS(SglTFImportWindow)
        : _glTFImportOptions(nullptr)
        , _WidgetWindow(nullptr)
        , _CurrentFile()
        {}

        SLATE_ARGUMENT(TSharedPtr<struct FglTFImportOptions>, glTFImportOptions)
        SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
        SLATE_ARGUMENT(FText, CurrentFile)
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
    void HandleMeshScaleRatio(float InNewValue);
    void HandleMeshInvertNormal(ECheckBoxState InCheckBoxState);
    void HandleMaterialImportMaterial(ECheckBoxState InCheckBoxState);

private:
    TWeakPtr<struct FglTFImportOptions> glTFImportOptions;
    TWeakPtr<SWindow> WidgetWindow;
};
