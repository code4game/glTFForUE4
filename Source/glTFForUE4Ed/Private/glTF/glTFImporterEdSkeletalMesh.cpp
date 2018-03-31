// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"

#include "glTF/glTFImporterEd.h"

#include "glTF/glTFImportOptions.h"

#include "libgltf/libgltf.h"

#include "Engine/SkeletalMesh.h"

#include "SkelImport.h"

#define LOCTEXT_NAMESPACE "FglTFForUE4EdModule"

USkeletalMesh* FglTFImporterEd::CreateSkeletalMesh(const TWeakPtr<FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::vector<std::shared_ptr<libgltf::SScene>>& InScenes, const class FglTFBuffers& InBuffers) const
{
    if (!InGlTF || InScenes.empty()) return nullptr;
    if (InputClass != USkeletalMesh::StaticClass() || !InputParent || !InputName.IsValid()) return nullptr;

    /// Create new static mesh
    USkeletalMesh* SkeletalMesh = NewObject<USkeletalMesh>(InputParent, InputClass, InputName, InputFlags);
    checkSlow(SkeletalMesh);
    if (!SkeletalMesh) return nullptr;

#if ENGINE_MINOR_VERSION < 19
    FSkeletalMeshResource* ImportedResource = SkeletalMesh->GetImportedResource();
    check(ImportedResource->LODModels.Num() == 0);
    ImportedResource->LODModels.Empty();
    new(ImportedResource->LODModels)FStaticLODModel();
#endif

    FSkeletalMeshImportData TempData;
    //TODO:s

    //SkeletalMesh->PostEditChange();
    return nullptr;
}

#undef LOCTEXT_NAMESPACE
