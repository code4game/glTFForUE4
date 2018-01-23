// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4PrivatePCH.h"
#include "glTF/glTFImportOptions.h"

FglTFImportOptions::FglTFImportOptions()
    : FilePathInOS(TEXT(""))
    , FilePathInEngine(TEXT(""))
    , bImportAllScenes(false)
    , bImportSkeleton(false)
    , bImportMaterial(false)
    , MeshScaleRatio(100.0f)
    , bInvertNormal(false)
    , bUseMikkTSpace(true)
    , bRecomputeNormals(false)
    , bRecomputeTangents(false)
{
    //
}

const FglTFImportOptions FglTFImportOptions::Default;
FglTFImportOptions FglTFImportOptions::Current;
