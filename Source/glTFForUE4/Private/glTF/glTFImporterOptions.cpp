// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4PrivatePCH.h"
#include "glTF/glTFImporterOptions.h"

UglTFImporterOptionsDetails::UglTFImporterOptionsDetails(const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
    , bImportSkeletalMesh(true)
    , bImportMaterial(true)
    , bImportTexture(true)
    , bImportAllScene(false)
    , bImportLevel(false)
    , ImportLevelTemplate()
    , MeshScaleRatio(1.0f)
    , bGenerateLightmapUVs(true)
    , bInvertNormal(false)
    , bUseMikkTSpace(true)
    , bRecomputeNormals(false)
    , bRecomputeTangents(false)
    , bRemoveDegenerates(false)
    , bBuildAdjacencyBuffer(false)
    , bUseFullPrecisionUVs(false)
    , bUseMaterialInstance(false)
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
