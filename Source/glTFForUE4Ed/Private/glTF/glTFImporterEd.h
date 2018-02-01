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
    static TSharedPtr<FglTFImporterEd> Get(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext);

public:
    FglTFImporterEd();
    virtual ~FglTFImporterEd();

public:
    virtual UObject* Create(const TWeakPtr<struct FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF) const override;

private:
    UStaticMesh* CreateStaticMesh(const TWeakPtr<struct FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::vector<std::shared_ptr<libgltf::SScene>>& InScenes, const class FglTFBufferFiles& InBufferFiles) const;
    bool GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const struct FMatrix& InMatrix, const std::shared_ptr<libgltf::SNode>& InNode, const class FglTFBufferFiles& InBufferFiles, struct FRawMesh& OutRawMesh, TArray<FglTFMaterialInfo>& InOutglTFMaterialInfos, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;
    bool GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const struct FMatrix& InMatrix, const std::shared_ptr<libgltf::SMesh>& InMesh, const class FglTFBufferFiles& InBufferFiles, struct FRawMesh& OutRawMesh, TArray<FglTFMaterialInfo>& InOutglTFMaterialInfos, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;
    bool GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const struct FMatrix& InMatrix, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const class FglTFBufferFiles& InBufferFiles, struct FRawMesh& OutRawMesh, int32 InMaterialId, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;

private:
    class UMaterial* CreateMaterial(const TWeakPtr<struct FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFMaterialInfo& InglTFMaterialInfo, class UMaterial* InOrigin) const;
    class UTexture* CreateTexture(const TWeakPtr<struct FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, int32 InTextureId, const FString& InPackageName) const;
};
