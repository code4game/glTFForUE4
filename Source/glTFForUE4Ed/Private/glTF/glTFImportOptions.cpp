#include "glTFForUE4EdPrivatePCH.h"
#include "glTFImportOptions.h"

FglTFImportOptions::FglTFImportOptions()
    : FilePathInOS(TEXT(""))
    , FilePathInEngine(TEXT(""))
    , MeshScaleRatio(100.0f)
    , bInvertNormal(false)
    , bImportAllScenes(false)
    , bImportMaterial(false)
    , bUseMikkTSpace(true)
    , bRecomputeNormals(false)
    , bRecomputeTangents(false)
{
    //
}

const FglTFImportOptions FglTFImportOptions::Default;
FglTFImportOptions FglTFImportOptions::Current;
