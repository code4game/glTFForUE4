// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4PrivatePCH.h"
#include "glTF/glTFImporterOptions.h"

UglTFImporterOptionsDetails::UglTFImporterOptionsDetails(const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
    , bImportAllScene(false)
    , bImportSkeletalMesh(true)
    , bImportMaterial(true)
    , bImportTexture(true)
    , MeshScaleRatio(1.0f)
    , bInvertNormal(false)
    , bUseMikkTSpace(true)
    , bRecomputeNormals(false)
    , bRecomputeTangents(false)
    , bRemoveDegenerates(false)
    , bBuildAdjacencyBuffer(false)
    , bUseFullPrecisionUVs(false)
    , bGenerateLightmapUVs(true)
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
