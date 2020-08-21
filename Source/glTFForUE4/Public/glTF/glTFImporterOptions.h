#pragma once

#include "glTFImporterOptions.generated.h"

UCLASS(config = glTFForUE4Settings, defaultconfig)
class GLTFFORUE4_API UglTFImporterOptionsDetails : public UObject
{
    GENERATED_UCLASS_BODY()

public:
    /// Import options
    UPROPERTY(EditAnywhere, Config, Category = "Import")
    bool bImportAllScene;
    
    UPROPERTY(EditAnywhere, Config, Category = "Import")
    bool bImportSkeletalMesh;

    UPROPERTY(EditAnywhere, Config, Category = "Import")
    bool bImportMaterial;

    UPROPERTY(EditAnywhere, Config, Category = "Import")
    bool bImportTexture;

    /// Mesh options
    UPROPERTY(EditAnywhere, Config, Category = "Mesh")
    bool bUseAbsolateTransform;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh")
    float MeshScaleRatio;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh")
    bool bInvertNormal;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh")
    bool bUseMikkTSpace;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh")
    bool bRecomputeNormals;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh")
    bool bRecomputeTangents;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh")
    bool bRemoveDegenerates;
    
    UPROPERTY(EditAnywhere, Config, Category = "Mesh")
    bool bBuildAdjacencyBuffer;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh")
    bool bUseFullPrecisionUVs;
    
    UPROPERTY(EditAnywhere, Config, Category = "Mesh")
    bool bGenerateLightmapUVs;
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

    UPROPERTY(EditAnywhere, Category = glTFForUE4Ed)
    UglTFImporterOptionsDetails* Details;
};
