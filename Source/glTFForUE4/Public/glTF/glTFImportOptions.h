#pragma once

#include "glTFImportOptions.generated.h"

UENUM()
enum class EglTFImportType : uint8
{
    None,
    StaticMesh,
    SkeletalMesh,
    Actor,
    Level,
};

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

    /// Static Mesh options
    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bIntegrateAllMeshsForStaticMesh;

    /// Skeletal Mesh options
    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bImportAnimationForSkeletalMesh;

    /// Material options
    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bImportMaterial;

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    bool bImportTexture;

    static const FglTFImportOptions Default;
    static FglTFImportOptions Current;
};
