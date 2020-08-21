// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4PrivatePCH.h"
#include "glTF/glTFImporterOptions.h"

UglTFImporterOptionsDetails::UglTFImporterOptionsDetails(const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
    , bBuildLevel(false)
    , bBuildLevelByTemplate(false)
    , BuildLevelTemplate()
    , bImportSkeletalMesh(true)
    , bImportMaterial(true)
    , bImportTexture(true)
    , bImportAllScene(false)
    , MeshScaleRatio(1.0f)
    , bGenerateLightmapUVs(true)
    , bApplyAbsolateTransform(false)
    , bInvertNormal(false)
    , bUseMikkTSpace(true)
    , bRecomputeNormals(false)
    , bRecomputeTangents(false)
    , bRemoveDegenerates(false)
    , bBuildAdjacencyBuffer(false)
    , bUseFullPrecisionUVs(false)
{
    //
}

FglTFImporterOptions::FglTFImporterOptions()
    : FilePathInOS(TEXT(""))
    , FilePathInEngine(TEXT(""))
    , Details(nullptr)
{
    //
}
