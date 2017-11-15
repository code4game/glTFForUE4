#include "glTFForUE4EdPrivatePCH.h"
#include "glTFImportOptions.h"

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
