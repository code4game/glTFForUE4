// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "Styling/SlateTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "glTF/glTFImportOptions.h"

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
        : _glTFImportOptions(nullptr)
        , _WidgetWindow(nullptr)
        , _ImportTypes()
        , _bHasAnimation(false)
        {}

        SLATE_ARGUMENT(TSharedPtr<struct FglTFImportOptions>, glTFImportOptions)
        SLATE_ARGUMENT(TSharedPtr<class SWindow>, WidgetWindow)
        SLATE_ARGUMENT(TArray<TSharedPtr<EglTFImportType>>, ImportTypes)
        SLATE_ARGUMENT(bool, bHasAnimation)
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

    void HandleImportType(const TSharedPtr<EglTFImportType> InImportType, ESelectInfo::Type InSelectInfo);
    TSharedRef<SWidget> GenerateImportType(TSharedPtr<EglTFImportType> InImportType) const;
    FText GetImportTypeText() const;
    FText GetImportTypeText(EglTFImportType InImportType) const;

    void HandleMeshScaleRatio(float InNewValue);
    void HandleMeshInvertNormal(ECheckBoxState InCheckBoxState);
    void HandleMeshUseMikkTSpace(ECheckBoxState InCheckBoxState);
    void HandleMeshRecomputeNormals(ECheckBoxState InCheckBoxState);
    void HandleMeshRecomputeTangents(ECheckBoxState InCheckBoxState);

    bool CanHandleIntegrateAllMeshsForStaticMesh() const;
    ECheckBoxState CheckHandleIntegrateAllMeshsForStaticMesh() const;
    void HandleIntegrateAllMeshsForStaticMesh(ECheckBoxState InCheckBoxState);

    bool CanHandleImportAnimationForSkeletalMesh() const;
    ECheckBoxState CheckHandleImportAnimationForSkeleton() const;
    void HandleImportAnimationForSkeletalMesh(ECheckBoxState InCheckBoxState);

    void HandleImportMaterial(ECheckBoxState InCheckBoxState);
    void HandleImportTexture(ECheckBoxState InCheckBoxState);

protected:
    bool HasAnimation() const;

protected:
    TWeakPtr<struct FglTFImportOptions> glTFImportOptions;
    TWeakPtr<class SWindow> WidgetWindow;
    TArray<TSharedPtr<EglTFImportType>> ImportTypes;
    bool bHasAnimation;
};
