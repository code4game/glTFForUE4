// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterEdLevel.h"

#include "glTF/glTFImporterOptions.h"

TSharedPtr<FglTFImporterEdLevel> FglTFImporterEdLevel::Get(UFactory* InFactory, UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, FFeedbackContext* InFeedbackContext)
{
    //
    return nullptr;
}

FglTFImporterEdLevel::FglTFImporterEdLevel()
    : Super()
{
    //
}

FglTFImporterEdLevel::~FglTFImporterEdLevel()
{
    //
}

ULevel* FglTFImporterEdLevel::CreateLevel(const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::vector<std::shared_ptr<libgltf::SScene>>& InScenes, const FglTFBuffers& InBuffers) const
{
    //
    return nullptr;
}
