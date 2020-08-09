// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "glTF/glTFImporterEd.h"

class FglTFImporterEdMaterial : public FglTFImporterEd
{
    typedef FglTFImporterEd Super;

public:
    static TSharedPtr<FglTFImporterEdMaterial> Get(class UFactory* InFactory, UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext);

protected:
    FglTFImporterEdMaterial();

public:
    virtual ~FglTFImporterEdMaterial();

public:
    /// Create the material for mesh
    class UMaterial* CreateMaterial(const TWeakPtr<struct FglTFImporterOptions>& InglTFImporterOptions, const std::shared_ptr<libgltf::SGlTF>& InglTF, const class FglTFBuffers& InBuffers, const FglTFMaterialInfo& InglTFMaterialInfo, TMap<FString, class UTexture*>& InOutTextureLibrary, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;

private:
    bool ConstructSampleParameter(const TWeakPtr<struct FglTFImporterOptions>& InglTFImporterOptions, const std::shared_ptr<libgltf::SGlTF>& InglTF, const std::shared_ptr<libgltf::STextureInfo>& InglTFTextureInfo, const class FglTFBuffers& InBuffers, const FString& InParameterName, TMap<FString, class UTexture*>& InOutTextureLibrary, class UMaterialExpressionTextureSampleParameter* InSampleParameter, bool InIsNormalmap, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;
};
