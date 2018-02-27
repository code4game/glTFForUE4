// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "glTF/glTFImporter.h"

class FglTFImporterEd : public FglTFImporter
{
    typedef FglTFImporter Super;

    struct FglTFMaterialInfo
    {
        explicit FglTFMaterialInfo(int32 InId, FString InPrimitiveName);

        int32 Id;
        FString PrimitiveName;
    };

public:
    static TSharedPtr<FglTFImporterEd> Get(class UFactory* InFactory, UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext);

public:
    FglTFImporterEd();
    virtual ~FglTFImporterEd();

public:
    virtual UObject* Create(const TWeakPtr<struct FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBuffers& InglTFBuffers) const override;

private:
    UStaticMesh* CreateStaticMesh(const TWeakPtr<struct FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::vector<std::shared_ptr<libgltf::SScene>>& InScenes, const class FglTFBuffers& InBuffers) const;
    bool GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const struct FMatrix& InMatrix, const std::shared_ptr<libgltf::SNode>& InNode, const class FglTFBuffers& InBuffers, struct FRawMesh& OutRawMesh, TArray<FglTFMaterialInfo>& InOutglTFMaterialInfos, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;
    bool GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const struct FMatrix& InMatrix, const std::shared_ptr<libgltf::SMesh>& InMesh, const class FglTFBuffers& InBuffers, struct FRawMesh& OutRawMesh, TArray<FglTFMaterialInfo>& InOutglTFMaterialInfos, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;
    bool GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const struct FMatrix& InMatrix, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const class FglTFBuffers& InBuffers, struct FRawMesh& OutRawMesh, int32 InMaterialIndex, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;

private:
    class UMaterial* CreateMaterial(const TWeakPtr<struct FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InglTF, const class FglTFBuffers& InBuffers, const FglTFMaterialInfo& InglTFMaterialInfo, class UMaterial* InOrigin, TMap<FString, class UTexture*>& InOutTextureLibrary, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;
    bool ConstructSampleParameter(const TWeakPtr<struct FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InglTF, const std::shared_ptr<libgltf::STextureInfo>& InglTFTextureInfo, const class FglTFBuffers& InBuffers, const FString& InParameterName, TMap<FString, class UTexture*>& InOutTextureLibrary, class UMaterialExpressionTextureSampleParameter* InSampleParameter, bool InIsNormalmap, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;
    class UTexture* CreateTexture(const TWeakPtr<struct FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InglTF, const std::shared_ptr<libgltf::STexture>& InglTFTexture, const class FglTFBuffers& InBuffers, const FString& InTextureName, bool InIsNormalmap, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;

private:
    class UFactory* InputFactory;
};
