// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

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
    /// import a static mesh
    class UStaticMesh* CreateStaticMesh(const TWeakPtr<struct FglTFImporterOptions>& InglTFImporterOptions
        , const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SGlTFId>& InMeshId
        , const FTransform& InNodeAbsoluteTransform, const class FglTFBuffers& InBuffers
        , FglTFImporterCollection& InOutglTFImporterCollection) const;

private:
    bool GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMesh>& InMesh
        , const FTransform& InNodeAbsoluteTransform, const class FglTFBuffers& InBuffers
        , struct FRawMesh& OutRawMesh, TArray<FglTFMaterialInfo>& InOutglTFMaterialInfos
        , FglTFImporterCollection& InOutglTFImporterCollection
        , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;
    bool GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive
        , const FTransform& InNodeAbsoluteTransform, const class FglTFBuffers& InBuffers
        , struct FRawMesh& OutRawMesh, int32 InMaterialIndex
        , FglTFImporterCollection& InOutglTFImporterCollection
        , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;
};
