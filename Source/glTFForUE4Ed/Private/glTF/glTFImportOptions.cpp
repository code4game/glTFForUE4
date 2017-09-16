#include "glTFForUE4EdPrivatePCH.h"
#include "glTFImportOptions.h"

FglTFImportOptions::FglTFImportOptions()
    : FilePathInOS(TEXT(""))
    , FilePathInEngine(TEXT(""))
    , MeshScaleRatio(1.0f)
    , bInvertNormal(false)
    , bImportMaterial(true)
{
    //
}
