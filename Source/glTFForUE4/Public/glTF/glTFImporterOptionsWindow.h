// Copyright(c) 2016 - 2023 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "Styling/SlateTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

namespace libgltf
{
    struct SGlTF;
}

class GLTFFORUE4_API SglTFImporterOptionsWindow : public SCompoundWidget
{
public:
    static TSharedPtr<struct FglTFImporterOptions> Open(const FString& InFilePathInOS, const FString& InFilePathInEngine, const libgltf::SGlTF& InGlTF, bool& OutCancel);

public:
    SLATE_BEGIN_ARGS(SglTFImporterOptionsWindow)
        : _glTFImporterOptions(nullptr)
        , _WidgetWindow(nullptr)
        {}

        SLATE_ARGUMENT(TSharedPtr<struct FglTFImporterOptions>, glTFImporterOptions)
        SLATE_ARGUMENT(TSharedPtr<class SWindow>, WidgetWindow)
    SLATE_END_ARGS()

public:
    SglTFImporterOptionsWindow();

public:
    virtual void Construct(const FArguments& InArgs);

public:
    virtual bool SupportsKeyboardFocus() const override;
    virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

public:
    TSharedPtr<struct FglTFImporterOptions> GetImportOptions();

protected:
    bool CanImport() const;
    FReply OnImport();
    FReply OnCancel();

protected:
    TWeakPtr<struct FglTFImporterOptions> glTFImporterOptions;
    TWeakPtr<class SWindow> WidgetWindow;
};
