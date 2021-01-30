// Copyright(c) 2016 - 2021 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "glTF/glTFImporterEd.h"

class FglTFImporterEdSkeletalMesh : public FglTFImporterEd
{
    typedef FglTFImporterEd Super;

public:
    static TSharedPtr<FglTFImporterEdSkeletalMesh> Get(class UFactory* InFactory, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext);

protected:
    FglTFImporterEdSkeletalMesh();

public:
    virtual ~FglTFImporterEdSkeletalMesh();

public:
    /// Import a skeletal mesh
    class USkeletalMesh* CreateSkeletalMesh(
        const TWeakPtr<struct FglTFImporterOptions>& InglTFImporterOptions,
        const std::shared_ptr<libgltf::SGlTF>& InGlTF,
        const int32 InNodeId,
        const std::shared_ptr<libgltf::SGlTFId>& InMeshId,
        const std::shared_ptr<libgltf::SGlTFId>& InSkinId,
        const class FglTFBuffers& InBuffers,
        const FTransform& InNodeTransform,
        struct FglTFImporterCollection& InOutglTFImporterCollection) const;

private:
    bool GenerateSkeletalMeshImportData(const std::shared_ptr<libgltf::SGlTF>& InGlTF,
        const int32 InNodeId,
        const std::shared_ptr<libgltf::SMesh>& InMesh,
        const std::shared_ptr<libgltf::SSkin>& InSkin,
        const class FglTFBuffers& InBuffers,
        const FString& InMeshName,
        const FTransform& InNodeTransform,
        class FSkeletalMeshImportData& OutSkeletalMeshImportData,
        TArray<FMatrix>& OutInverseBindMatrices,
        TMap<int32, FString>& OutNodeIndexToBoneNames,
        TArray<int32>& InOutMaterialIds,
        const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper,
        struct FglTFImporterCollection& InOutglTFImporterCollection) const;
    bool GenerateSkeletalMeshImportData(const std::shared_ptr<libgltf::SGlTF>& InGlTF,
        const std::shared_ptr<libgltf::SMesh>& InMesh,
        const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive,
        const class FglTFBuffers& InBuffers,
        class FSkeletalMeshImportData& OutSkeletalMeshImportData,
        TMap<int32, FString>& OutNodeIndexToBoneNames,
        const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper,
        struct FglTFImporterCollection& InOutglTFImporterCollection) const;
    bool GenerateSkeletalMeshImportData(const std::shared_ptr<libgltf::SGlTF>& InGlTF,
        const std::shared_ptr<libgltf::SSkin>& InSkin,
        const class FglTFBuffers& InBuffers,
        const FTransform& InNodeTransform,
        class FSkeletalMeshImportData& OutSkeletalMeshImportData, TArray<FMatrix>& OutInverseBindMatrices,
        TMap<int32, FString>& OutNodeIndexToBoneNames,
        const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper,
        struct FglTFImporterCollection& InOutglTFImporterCollection) const;
};
