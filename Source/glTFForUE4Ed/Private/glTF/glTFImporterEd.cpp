// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterEd.h"

#include "glTF/glTFImportOptions.h"
#include "glTF/glTFImporterEdStaticMesh.h"
#include "glTF/glTFImporterEdSkeletalMesh.h"

#define LOCTEXT_NAMESPACE "glTFForUE4EdModule"

FglTFImporterEd::FglTFMaterialInfo::FglTFMaterialInfo(int32 InId, FString InPrimitiveName)
    : Id(InId)
    , PrimitiveName(InPrimitiveName)
{
    //
}

TSharedPtr<FglTFImporterEd> FglTFImporterEd::Get(UFactory* InFactory, UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext)
{
    TSharedPtr<FglTFImporterEd> glTFImporterEd = MakeShareable(new FglTFImporterEd);
    glTFImporterEd->Set(InClass, InParent, InName, InFlags, InFeedbackContext);
    glTFImporterEd->InputFactory = InFactory;
    return glTFImporterEd;
}

FglTFImporterEd::FglTFImporterEd()
    : Super()
    , InputFactory(nullptr)
{
    //
}

FglTFImporterEd::~FglTFImporterEd()
{
    //
}

UObject* FglTFImporterEd::Create(const TWeakPtr<FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBuffers& InglTFBuffers) const
{
    if (!InGlTF)
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Invalid InGlTF!"));
        return nullptr;
    }

    if (!InGlTF->asset || InGlTF->asset->version != TEXT("2.0"))
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Invalid version: %s!"), !(InGlTF->asset) ? TEXT("none") : InGlTF->asset->version.c_str());
        return nullptr;
    }

    const TSharedPtr<FglTFImportOptions> glTFImportOptions = InglTFImportOptions.Pin();

    FlushRenderingCommands();

    std::vector<std::shared_ptr<libgltf::SScene>> Scenes;
    if (InGlTF->scene)
    {
        Scenes.push_back(InGlTF->scenes[(int32)(*InGlTF->scene)]);
    }
    else if (InGlTF->scenes.size() > 0)
    {
        for (const std::shared_ptr<libgltf::SScene>& Scene : InGlTF->scenes)
        {
            Scenes.push_back(Scene);
        }
    }

    switch (glTFImportOptions->ImportType)
    {
    case EglTFImportType::StaticMesh:
        return FglTFImporterEdStaticMesh::Get(InputFactory, InputClass, InputParent, InputName, InputFlags, FeedbackContext)->CreateStaticMesh(InglTFImportOptions, InGlTF, Scenes, InglTFBuffers);

    case EglTFImportType::SkeletalMesh:
        return FglTFImporterEdSkeletalMesh::Get(InputFactory, InputClass, InputParent, InputName, InputFlags, FeedbackContext)->CreateSkeletalMesh(InglTFImportOptions, InGlTF, Scenes, InglTFBuffers);

    case EglTFImportType::Actor:
    case EglTFImportType::Level:
    default:
        break;
    }
    return nullptr;
}

#undef LOCTEXT_NAMESPACE
