#pragma once

#include "glTFImporterOptions.generated.h"

UCLASS(config = glTFForUE4Settings, defaultconfig)
class GLTFFORUE4_API UglTFImporterOptionsDetails : public UObject
{
    GENERATED_UCLASS_BODY()

public:
    /// common options
    UPROPERTY(EditAnywhere, Config, Category = "Common")
    bool bBuildLevel;

    UPROPERTY(EditAnywhere, Config, Category = "Common", meta = (EditCondition = bBuildLevel))
    bool bBuildLevelByTemplate;
    
    UPROPERTY(EditAnywhere, Config, Category = "Common", meta = (EditCondition = bBuildLevelByTemplate, AllowedClasses = "World"))
    FStringAssetReference BuildLevelTemplate;

    UPROPERTY(EditAnywhere, Config, Category = "Common")
    bool bImportSkeletalMesh;

    UPROPERTY(EditAnywhere, Config, Category = "Common")
    bool bImportMaterial;

    UPROPERTY(EditAnywhere, Config, Category = "Common")
    bool bImportTexture;

    UPROPERTY(EditAnywhere, Config, AdvancedDisplay, Category = "Common")
    bool bImportAllScene;
    
    /// mesh options
    UPROPERTY(EditAnywhere, Config, Category = "Mesh")
    float MeshScaleRatio;
    
    UPROPERTY(EditAnywhere, Config, Category = "Mesh")
    bool bGenerateLightmapUVs;
    
    UPROPERTY(EditAnywhere, Config, Category = "Mesh")
    bool bApplyAbsolateTransform;

    UPROPERTY(EditAnywhere, Config, AdvancedDisplay, Category = "Mesh")
    bool bInvertNormal;

    UPROPERTY(EditAnywhere, Config, AdvancedDisplay, Category = "Mesh")
    bool bUseMikkTSpace;

    UPROPERTY(EditAnywhere, Config, AdvancedDisplay, Category = "Mesh")
    bool bRecomputeNormals;

    UPROPERTY(EditAnywhere, Config, AdvancedDisplay, Category = "Mesh")
    bool bRecomputeTangents;

    UPROPERTY(EditAnywhere, Config, AdvancedDisplay, Category = "Mesh")
    bool bRemoveDegenerates;
    
    UPROPERTY(EditAnywhere, Config, AdvancedDisplay, Category = "Mesh")
    bool bBuildAdjacencyBuffer;

    UPROPERTY(EditAnywhere, Config, AdvancedDisplay, Category = "Mesh")
    bool bUseFullPrecisionUVs;
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
