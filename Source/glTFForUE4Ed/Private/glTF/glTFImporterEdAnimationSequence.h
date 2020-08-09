// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "glTF/glTFImporterEd.h"

class FglTFImporterEdAnimationSequence : public FglTFImporterEd
{
    typedef FglTFImporterEd Super;

public:
    static TSharedPtr<FglTFImporterEdAnimationSequence> Get(class UFactory* InFactory, UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext);

protected:
    FglTFImporterEdAnimationSequence();

public:
    virtual ~FglTFImporterEdAnimationSequence();

public:
    class UAnimSequence* CreateAnimationSequence(const TWeakPtr<struct FglTFImporterOptions>& InglTFImporterOptions, const std::shared_ptr<libgltf::SGlTF>& InglTF
        , const TArray<FTransform>& InNodeRelativeTransforms, const TArray<FTransform>& InNodeAbsoluteTransforms, const class FglTFBuffers& InBuffers, const TMap<int32, FString>& InNodeIndexToBoneNames
        , class USkeletalMesh* InSkeletalMesh, class USkeleton* InSkeleton
        , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const;
};
