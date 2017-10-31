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

    /// Build setting
    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bImportAllScenes;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bImportSkeleton;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bImportMaterial;

    /// Mesh
    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    FVector MeshScaleRatio;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bInvertNormal;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bUseMikkTSpace;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bRecomputeNormals;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bRecomputeTangents;

    /// Material

    static const FglTFImportOptions Default;
    static FglTFImportOptions Current;
};
