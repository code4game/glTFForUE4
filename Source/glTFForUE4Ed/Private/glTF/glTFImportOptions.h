#pragma once

#include "glTFImportOptions.generated.h"

USTRUCT()
struct FglTFImportOptions
{
    GENERATED_USTRUCT_BODY()

    FglTFImportOptions();

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    FString FilePathInOS;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    FString FilePathInEngine;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    FVector MeshScaleRatio;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bInvertNormal;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bImportMaterial;

    /// Build setting
    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bUseMikkTSpace;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bRecomputeNormals;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bRecomputeTangents;

    static const FglTFImportOptions Default;
    static FglTFImportOptions Current;
};
