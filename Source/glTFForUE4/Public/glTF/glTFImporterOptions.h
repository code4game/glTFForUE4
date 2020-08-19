#pragma once

#include "glTFImporterOptions.generated.h"

UENUM()
enum class EglTFImportType : uint8
{
    None,
    StaticMesh,
    SkeletalMesh,
    Level,
};

USTRUCT()
struct GLTFFORUE4_API FglTFImporterOptions
{
    GENERATED_USTRUCT_BODY()

    FglTFImporterOptions();

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    FString FilePathInOS;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    FString FilePathInEngine;

    /// Import options
    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    EglTFImportType ImportType;

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
    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bImportMaterial;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bImportTexture;

    static const FglTFImporterOptions Default;
    static FglTFImporterOptions Current;
};
