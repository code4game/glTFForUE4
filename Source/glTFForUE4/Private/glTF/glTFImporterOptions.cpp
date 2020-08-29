// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4PrivatePCH.h"
#include "glTF/glTFImporterOptions.h"

FglTFImporterOptionsDetailsStored::FglTFImporterOptionsDetailsStored()
    : bImportSkeletalMesh(true)
    , bImportAnimation(true)
    , bImportMaterial(true)
    , bImportTexture(true)
    , bImportAllScene(false)
    , MeshScaleRatio(1.0f)
    , bApplyAbsoluteTransform(false)
    , bGenerateLightmapUVs(true)
    , bInvertNormal(false)
    , bUseMikkTSpace(true)
    , bRecomputeNormals(false)
    , bRecomputeTangents(false)
    , bRemoveDegenerates(false)
    , bBuildAdjacencyBuffer(false)
    , bUseFullPrecisionUVs(false)
    , bCreatePhysicsAsset(true)
    , bUseMaterialInstance(true)
{
    //
}

UglTFImporterOptionsDetails::UglTFImporterOptionsDetails(const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
    , bImportSkeletalMesh(true)
    , bImportAnimation(true)
    , bImportMaterial(true)
    , bImportTexture(true)
    , bImportAllScene(false)
    , bImportLevel(false)
    , ImportLevelTemplate()
    , MeshScaleRatio(1.0f)
    , bApplyAbsoluteTransform(false)
    , bGenerateLightmapUVs(true)
    , bInvertNormal(false)
    , bUseMikkTSpace(true)
    , bRecomputeNormals(false)
    , bRecomputeTangents(false)
    , bRemoveDegenerates(false)
    , bBuildAdjacencyBuffer(false)
    , bUseFullPrecisionUVs(false)
    , bCreatePhysicsAsset(true)
    , bUseMaterialInstance(true)
{
    //
}

void UglTFImporterOptionsDetails::Get(FglTFImporterOptionsDetailsStored& OutDetailsStored) const
{
    OutDetailsStored.bImportSkeletalMesh = bImportSkeletalMesh;
    OutDetailsStored.bImportAnimation = bImportAnimation;
    OutDetailsStored.bImportMaterial = bImportMaterial;
    OutDetailsStored.bImportTexture = bImportTexture;
    OutDetailsStored.bImportAllScene = bImportAllScene;
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
    OutDetailsStored.bCreatePhysicsAsset = bCreatePhysicsAsset;
    OutDetailsStored.bUseMaterialInstance = bUseMaterialInstance;
}

void UglTFImporterOptionsDetails::Set(const FglTFImporterOptionsDetailsStored& InDetailsStored)
{
    bImportSkeletalMesh = InDetailsStored.bImportSkeletalMesh;
    bImportAnimation = InDetailsStored.bImportAnimation;
    bImportMaterial = InDetailsStored.bImportMaterial;
    bImportTexture = InDetailsStored.bImportTexture;
    bImportAllScene = InDetailsStored.bImportAllScene;
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
    bCreatePhysicsAsset = InDetailsStored.bCreatePhysicsAsset;
    bUseMaterialInstance = InDetailsStored.bUseMaterialInstance;
}

FglTFImporterOptions::FglTFImporterOptions()
    : FilePathInOS(TEXT(""))
    , FilePathInEngine(TEXT(""))
    , DetailsStored()
    , Details(nullptr)
{
    //
}
