// Copyright(c) 2016 - 2022 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4PrivatePCH.h"
#include "glTF/glTFImporterOptions.h"

#if GLTFFORUE_ENGINE_VERSION < 426
#else
#include <Misc/SecureHash.h>
#endif

FglTFImporterOptionsDetailsStored::FglTFImporterOptionsDetailsStored()
    : bImportAllScene(false)
    , bImportStaticMesh(true)
    , bImportSkeletalMesh(true)
    , bImportMaterial(true)
    , bImportTexture(true)
    , MeshScaleRatio(100.0f)
    , bApplyAbsoluteTransform(false)
    , bGenerateLightmapUVs(true)
    , bInvertNormal(false)
    , bUseMikkTSpace(true)
    , bRecomputeNormals(false)
    , bRecomputeTangents(false)
    , bRemoveDegenerates(false)
    , bBuildAdjacencyBuffer(false)
    , bUseFullPrecisionUVs(false)
    , bImportAnimation(true)
    , bImportMorphTarget(true)
    , bCreatePhysicsAsset(true)
    , bUseMaterialInstance(true)
{
    //
}

UglTFImporterOptionsDetails::UglTFImporterOptionsDetails(const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
    , bImportAllScene(false)
    , bImportStaticMesh(true)
    , bImportSkeletalMesh(true)
    , bImportMaterial(true)
    , bImportTexture(true)
    , bImportLevel(false)
    , MeshScaleRatio(100.0f)
    , bApplyAbsoluteTransform(false)
    , bGenerateLightmapUVs(true)
    , bInvertNormal(false)
    , bUseMikkTSpace(true)
    , bRecomputeNormals(false)
    , bRecomputeTangents(false)
    , bRemoveDegenerates(false)
    , bBuildAdjacencyBuffer(false)
    , bUseFullPrecisionUVs(false)
    , bImportAnimation(true)
    , bImportMorphTarget(true)
    , bCreatePhysicsAsset(true)
    , bUseMaterialInstance(true)
    , bImportLightInLevel(false)
    , bImportCameraInLevel(false)
    , ImportLevelTemplate()
{
    //
}

void UglTFImporterOptionsDetails::Get(FglTFImporterOptionsDetailsStored& OutDetailsStored) const
{
    OutDetailsStored.bImportAllScene = bImportAllScene;
    OutDetailsStored.bImportStaticMesh = bImportStaticMesh;
    OutDetailsStored.bImportSkeletalMesh = bImportSkeletalMesh;
    OutDetailsStored.bImportMaterial = bImportMaterial;
    OutDetailsStored.bImportTexture = bImportTexture;
    OutDetailsStored.MeshScaleRatio = MeshScaleRatio;
    OutDetailsStored.bApplyAbsoluteTransform = bApplyAbsoluteTransform;
    OutDetailsStored.bGenerateLightmapUVs = bGenerateLightmapUVs;
    OutDetailsStored.bInvertNormal = bInvertNormal;
    OutDetailsStored.bUseMikkTSpace = bUseMikkTSpace;
    OutDetailsStored.bRecomputeNormals = bRecomputeNormals;
    OutDetailsStored.bRecomputeTangents = bRecomputeTangents;
    OutDetailsStored.bRemoveDegenerates = bRemoveDegenerates;
    OutDetailsStored.bBuildAdjacencyBuffer = bBuildAdjacencyBuffer;
    OutDetailsStored.bUseFullPrecisionUVs = bUseFullPrecisionUVs;
    OutDetailsStored.bImportAnimation = bImportAnimation;
    OutDetailsStored.bImportMorphTarget = bImportMorphTarget;
    OutDetailsStored.bCreatePhysicsAsset = bCreatePhysicsAsset;
    OutDetailsStored.bUseMaterialInstance = bUseMaterialInstance;
}

void UglTFImporterOptionsDetails::Set(const FglTFImporterOptionsDetailsStored& InDetailsStored)
{
    bImportAllScene = InDetailsStored.bImportAllScene;
    bImportStaticMesh = InDetailsStored.bImportStaticMesh;
    bImportSkeletalMesh = InDetailsStored.bImportSkeletalMesh;
    bImportMaterial = InDetailsStored.bImportMaterial;
    bImportTexture = InDetailsStored.bImportTexture;
    MeshScaleRatio = InDetailsStored.MeshScaleRatio;
    bApplyAbsoluteTransform = InDetailsStored.bApplyAbsoluteTransform;
    bGenerateLightmapUVs = InDetailsStored.bGenerateLightmapUVs;
    bInvertNormal = InDetailsStored.bInvertNormal;
    bUseMikkTSpace = InDetailsStored.bUseMikkTSpace;
    bRecomputeNormals = InDetailsStored.bRecomputeNormals;
    bRecomputeTangents = InDetailsStored.bRecomputeTangents;
    bRemoveDegenerates = InDetailsStored.bRemoveDegenerates;
    bBuildAdjacencyBuffer = InDetailsStored.bBuildAdjacencyBuffer;
    bUseFullPrecisionUVs = InDetailsStored.bUseFullPrecisionUVs;
    bImportAnimation = InDetailsStored.bImportAnimation;
    bImportMorphTarget = InDetailsStored.bImportMorphTarget;
    bCreatePhysicsAsset = InDetailsStored.bCreatePhysicsAsset;
    bUseMaterialInstance = InDetailsStored.bUseMaterialInstance;
}

FglTFImporterOptions::FglTFImporterOptions()
    : FilePathInOS(TEXT(""))
    , FilePathInEngine(TEXT(""))
    , DetailsStored()
    , Details(nullptr)
#if GLTFFORUE_ENGINE_VERSION < 414
#else
    , FileHash(nullptr)
#endif
{
    //
}
