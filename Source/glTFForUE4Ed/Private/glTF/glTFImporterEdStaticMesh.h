// Copyright(c) 2016 - 2022 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "glTF/glTFImporterEd.h"

class FglTFImporterEdStaticMesh : public FglTFImporterEd
{
    typedef FglTFImporterEd Super;

  public:
    static TSharedPtr<FglTFImporterEdStaticMesh> Get(class UFactory*         InFactory, //
                                                     UObject*                InParent,
                                                     FName                   InName,
                                                     EObjectFlags            InFlags,
                                                     class FFeedbackContext* InFeedbackContext);

  protected:
    FglTFImporterEdStaticMesh();

  public:
    virtual ~FglTFImporterEdStaticMesh();

  public:
    /// import a static mesh
    class UStaticMesh* CreateStaticMesh(const TWeakPtr<struct FglTFImporterOptions>& InglTFImporterOptions,
                                        const std::shared_ptr<libgltf::SGlTF>&       InGlTF,
                                        const std::shared_ptr<libgltf::SGlTFId>&     InMeshId,
                                        const class FglTFBuffers&                    InBuffers,
                                        const FTransform&                            InNodeAbsoluteTransform,
                                        struct FglTFImporterCollection&              InOutglTFImporterCollection) const;

  private:
    bool GenerateRawMesh(const TSharedPtr<struct FglTFImporterOptions> InglTFImporterOptions,
                         const std::shared_ptr<libgltf::SGlTF>&        InGlTF,
                         const std::shared_ptr<libgltf::SMesh>&        InMesh,
                         const class FglTFBuffers&                     InBuffers,
                         const FTransform&                             InNodeAbsoluteTransform,
                         struct FRawMesh&                              OutRawMesh,
                         TArray<int32>&                                InOutglTFMaterialIds,
                         const glTFForUE4::FFeedbackTaskWrapper&       InFeedbackTaskWrapper,
                         FglTFImporterCollection&                      InOutglTFImporterCollection) const;
    bool GenerateRawMesh(const TSharedPtr<struct FglTFImporterOptions>   InglTFImporterOptions,
                         const std::shared_ptr<libgltf::SGlTF>&          InGlTF,
                         const std::shared_ptr<libgltf::SMesh>&          InMesh,
                         const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive,
                         const class FglTFBuffers&                       InBuffers,
                         const FTransform&                               InNodeAbsoluteTransform,
                         struct FRawMesh&                                OutRawMesh,
                         int32                                           InMaterialIndex,
                         const glTFForUE4::FFeedbackTaskWrapper&         InFeedbackTaskWrapper,
                         FglTFImporterCollection&                        InOutglTFImporterCollection) const;
};
