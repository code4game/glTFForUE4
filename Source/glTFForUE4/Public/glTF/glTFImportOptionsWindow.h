// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "SlateBasics.h"

namespace libgltf
{
    struct SGlTF;
}

class GLTFFORUE4_API SglTFImportOptionsWindow : public SCompoundWidget
{
public:
    static TSharedPtr<struct FglTFImportOptions> Open(const FString& InFilePathInOS, const FString& InFilePathInEngine, const libgltf::SGlTF& InGlTF, bool& OutCancel);

public:
    SLATE_BEGIN_ARGS(SglTFImportOptionsWindow)
        : _GlTF(nullptr)
        , _glTFImportOptions(nullptr)
        , _WidgetWindow(nullptr)
        {}

        SLATE_ARGUMENT(TSharedPtr<libgltf::SGlTF>, GlTF)
        SLATE_ARGUMENT(TSharedPtr<struct FglTFImportOptions>, glTFImportOptions)
        SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
    SLATE_END_ARGS()

public:
    SglTFImportOptionsWindow();

public:
    virtual void Construct(const FArguments& InArgs);

public:
    virtual bool SupportsKeyboardFocus() const override;
    virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

public:
    TSharedPtr<struct FglTFImportOptions> GetImportOptions();

protected:
    bool CanImport() const;
    FReply OnImport();
    FReply OnCancel();
    void HandleImportScene(ECheckBoxState InCheckBoxState);
    void HandleImportSkeleton(ECheckBoxState InCheckBoxState);
    void HandleImportMaterial(ECheckBoxState InCheckBoxState);
    void HandleMeshScaleRatio(float InNewValue);
    void HandleMeshInvertNormal(ECheckBoxState InCheckBoxState);
    void HandleMeshUseMikkTSpace(ECheckBoxState InCheckBoxState);
    void HandleMeshRecomputeNormals(ECheckBoxState InCheckBoxState);
    void HandleMeshRecomputeTangents(ECheckBoxState InCheckBoxState);

protected:
    TWeakPtr<libgltf::SGlTF> GlTF;
    TWeakPtr<struct FglTFImportOptions> glTFImportOptions;
    TWeakPtr<SWindow> WidgetWindow;
};
