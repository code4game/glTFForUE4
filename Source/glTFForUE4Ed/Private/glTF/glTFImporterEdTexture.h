// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "glTF/glTFImporterEd.h"

namespace libgltf
{
    struct STexture;
}

class FglTFImporterEdTexture : public FglTFImporterEd
{
    typedef FglTFImporterEd Super;

public:
    static TSharedPtr<FglTFImporterEdTexture> Get(class UFactory* InFactory, UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext);

protected:
    FglTFImporterEdTexture();

public:
    virtual ~FglTFImporterEdTexture();

public:
    /// Create the texture for material
    class UTexture* CreateTexture(const TWeakPtr<struct FglTFImporterOptions>& InglTFImporterOptions, const std::shared_ptr<libgltf::SGlTF>& InglTF, const std::shared_ptr<libgltf::STexture>& InglTFTexture, const class FglTFBuffers& InBuffers, const FString& InTextureName, bool InIsNormalmap, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;
};
