// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "glTF/glTFImporterEd.h"

class FglTFImporterEdSkeletalMesh : public FglTFImporterEd
{
    typedef FglTFImporterEd Super;

public:
    static TSharedPtr<FglTFImporterEdSkeletalMesh> Get(class UFactory* InFactory, UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext);

protected:
    FglTFImporterEdSkeletalMesh();

public:
    virtual ~FglTFImporterEdSkeletalMesh();

public:
    /// Import a skeletal mesh
    class USkeletalMesh* CreateSkeletalMesh(const TWeakPtr<struct FglTFImporterOptions>& InglTFImporterOptions
        , const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::vector<std::shared_ptr<libgltf::SScene>>& InScenes
        , const class FglTFBuffers& InBuffers) const;

private:
    TArray<class USkeletalMesh*> CreateSkeletalMesh(const TWeakPtr<struct FglTFImporterOptions>& InglTFImporterOptions
        , const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SNode>& InNode
        , const TArray<int32>& InNodeParentIndices, const TArray<FTransform>& InNodeRelativeTransforms, const TArray<FTransform>& InNodeAbsoluteTransforms, const class FglTFBuffers& InBuffers) const;
    class USkeletalMesh* CreateSkeletalMesh(const TWeakPtr<struct FglTFImporterOptions>& InglTFImporterOptions
        , const std::shared_ptr<libgltf::SGlTF>& InGlTF, int32 InMeshId, const std::shared_ptr<libgltf::SMesh>& InMesh, const std::shared_ptr<libgltf::SSkin>& InSkin
        , const TArray<int32>& InNodeParentIndices, const TArray<FTransform>& InNodeRelativeTransforms, const TArray<FTransform>& InNodeAbsoluteTransforms, const class FglTFBuffers& InBuffers) const;

private:
    bool GenerateSkeletalMeshImportData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMesh>& InMesh, const std::shared_ptr<libgltf::SSkin>& InSkin
        , const TArray<int32>& InNodeParentIndices, const TArray<FTransform>& InNodeRelativeTransforms, const TArray<FTransform>& InNodeAbsoluteTransforms, const class FglTFBuffers& InBuffers
        , class FSkeletalMeshImportData& OutSkeletalMeshImportData, TArray<FMatrix>& OutInverseBindMatrices, TMap<int32, FString>& OutNodeIndexToBoneNames, TArray<FglTFMaterialInfo>& InOutMaterialInfo
        , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;
    bool GenerateSkeletalMeshImportData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const std::shared_ptr<libgltf::SSkin>& InSkin
        , const TArray<int32>& InNodeParentIndices, const TArray<FTransform>& InNodeRelativeTransforms, const TArray<FTransform>& InNodeAbsoluteTransforms, const class FglTFBuffers& InBuffers
        , class FSkeletalMeshImportData& OutSkeletalMeshImportData, TArray<FMatrix>& OutInverseBindMatrices, TMap<int32, FString>& OutNodeIndexToBoneNames
        , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;
};
