// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "Styling/SlateTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "glTF/glTFImporterOptions.h"

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
        , _ImportTypes()
        , _bHasAnimation(false)
        {}

        SLATE_ARGUMENT(TSharedPtr<struct FglTFImporterOptions>, glTFImporterOptions)
        SLATE_ARGUMENT(TSharedPtr<class SWindow>, WidgetWindow)
        SLATE_ARGUMENT(TArray<TSharedPtr<EglTFImportType>>, ImportTypes)
        SLATE_ARGUMENT(bool, bHasAnimation)
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

    void HandleImportType(const TSharedPtr<EglTFImportType> InImportType, ESelectInfo::Type InSelectInfo);
    TSharedRef<SWidget> GenerateImportType(TSharedPtr<EglTFImportType> InImportType) const;
    FText GetImportTypeText() const;
    FText GetImportTypeText(EglTFImportType InImportType) const;

    void HandleMeshScaleRatio(float InNewValue);
    void HandleMeshInvertNormal(ECheckBoxState InCheckBoxState);
    void HandleMeshUseMikkTSpace(ECheckBoxState InCheckBoxState);
    void HandleMeshRecomputeNormals(ECheckBoxState InCheckBoxState);
    void HandleMeshRecomputeTangents(ECheckBoxState InCheckBoxState);
    void HandleMeshRemoveDegenerates(ECheckBoxState InCheckBoxState);
    void HandleMeshBuildAdjacencyBuffer(ECheckBoxState InCheckBoxState);
    void HandleMeshUseFullPrecisionUVs(ECheckBoxState InCheckBoxState);
    void HandleMeshGenerateLightmapUVs(ECheckBoxState InCheckBoxState);

    void HandleImportMaterial(ECheckBoxState InCheckBoxState);
    void HandleImportTexture(ECheckBoxState InCheckBoxState);

protected:
    bool HasAnimation() const;

protected:
    TWeakPtr<struct FglTFImporterOptions> glTFImporterOptions;
    TWeakPtr<class SWindow> WidgetWindow;
    TArray<TSharedPtr<EglTFImportType>> ImportTypes;
    bool bHasAnimation;
};
