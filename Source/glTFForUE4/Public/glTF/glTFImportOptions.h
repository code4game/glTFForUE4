#pragma once

#include "glTFImportOptions.generated.h"

USTRUCT()
struct GLTFFORUE4_API FglTFImportOptions
{
    GENERATED_USTRUCT_BODY()

    FglTFImportOptions();

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    FString FilePathInOS;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    FString FilePathInEngine;

    /// Import options
    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bImportAsScene;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bImportAsSkeleton;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bImportMaterial;

    /// Mesh options
    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    float MeshScaleRatio;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bInvertNormal;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bUseMikkTSpace;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bRecomputeNormals;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bRecomputeTangents;

    /// Material options

    static const FglTFImportOptions Default;
    static FglTFImportOptions Current;
};
