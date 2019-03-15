// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "glTF/glTFImporterEd.h"

class FglTFImporterEdStaticMesh : public FglTFImporterEd
{
    typedef FglTFImporterEd Super;

public:
    static TSharedPtr<FglTFImporterEdStaticMesh> Get(class UFactory* InFactory, UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext);

protected:
    FglTFImporterEdStaticMesh();

public:
    virtual ~FglTFImporterEdStaticMesh();

public:
    /// Import a static mesh
    class UStaticMesh* CreateStaticMesh(const TWeakPtr<struct FglTFImporterOptions>& InglTFImporterOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::vector<std::shared_ptr<libgltf::SScene>>& InScenes, const class FglTFBuffers& InBuffers) const;

private:
    bool GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SNode>& InNode
        , const FTransform& InNodeAbsoluteTransform, const TArray<FTransform>& InNodeAbsoluteTransforms, const class FglTFBuffers& InBuffers
        , struct FRawMesh& OutRawMesh, TArray<FglTFMaterialInfo>& InOutglTFMaterialInfos
        , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;
    bool GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMesh>& InMesh
        , const FTransform& InNodeAbsoluteTransform, const class FglTFBuffers& InBuffers
        , struct FRawMesh& OutRawMesh, TArray<FglTFMaterialInfo>& InOutglTFMaterialInfos
        , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;
    bool GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive
        , const FTransform& InNodeAbsoluteTransform, const class FglTFBuffers& InBuffers
        , struct FRawMesh& OutRawMesh, int32 InMaterialIndex
        , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;
};
