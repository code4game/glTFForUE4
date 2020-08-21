// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "glTF/glTFImporterEd.h"

class FglTFImporterEdMaterial : public FglTFImporterEd
{
    typedef FglTFImporterEd Super;

public:
    static TSharedPtr<FglTFImporterEdMaterial> Get(class UFactory* InFactory, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext);

protected:
    FglTFImporterEdMaterial();

public:
    virtual ~FglTFImporterEdMaterial();

public:
    /// Create the material for mesh
    class UMaterialInterface* CreateMaterial(const TWeakPtr<struct FglTFImporterOptions>& InglTFImporterOptions
        , const std::shared_ptr<libgltf::SGlTF>& InglTF, const int32 InMaterialId, const class FglTFBuffers& InBuffers
        , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper
        , struct FglTFImporterCollection& InOutglTFImporterCollection) const;

private:
    bool ConstructSampleParameter(const TWeakPtr<struct FglTFImporterOptions>& InglTFImporterOptions
        , const std::shared_ptr<libgltf::SGlTF>& InglTF, const std::shared_ptr<libgltf::STextureInfo>& InglTFTextureInfo, const class FglTFBuffers& InBuffers
        , const FString& InParameterName, class UMaterialExpressionTextureSampleParameter* InOutSampleParameter, bool InIsNormalmap, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper
        , struct FglTFImporterCollection& InOutglTFImporterCollection) const;
    bool ConstructTextureParameter(const TWeakPtr<struct FglTFImporterOptions>& InglTFImporterOptions
        , const std::shared_ptr<libgltf::SGlTF>& InglTF, const std::shared_ptr<libgltf::STextureInfo>& InglTFTextureInfo, const class FglTFBuffers& InBuffers
        , const FString& InParameterName, class UTexture*& OutTexture, bool InIsNormalmap, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper
        , struct FglTFImporterCollection& InOutglTFImporterCollection) const;
};
