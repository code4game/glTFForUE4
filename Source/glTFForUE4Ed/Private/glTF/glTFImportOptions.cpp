#include "glTFForUE4EdPrivatePCH.h"
#include "glTFImportOptions.h"

FglTFImportOptions::FglTFImportOptions()
    : FilePathInOS(TEXT(""))
    , FilePathInEngine(TEXT(""))
    , MeshScaleRatio(100.0f)
    , bInvertNormal(false)
    , bImportMaterial(true)
    , bRecomputeNormals(true)
    , bRecomputeTangents(true)
{
    //
}

const FglTFImportOptions FglTFImportOptions::Default;
FglTFImportOptions FglTFImportOptions::Current;
