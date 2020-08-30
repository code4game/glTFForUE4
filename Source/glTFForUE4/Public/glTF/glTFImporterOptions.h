// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.
#pragma once

#include "glTFImporterOptions.generated.h"

USTRUCT()
struct GLTFFORUE4_API FglTFImporterOptionsDetailsStored
{
    GENERATED_USTRUCT_BODY()

    FglTFImporterOptionsDetailsStored();
    
    /// common options
    UPROPERTY(EditAnywhere, Config, Category = "Common", meta = (ToolTip = "Construct the skeletal mesh when node has skin"))
    bool bImportSkeletalMesh;
    
    UPROPERTY(EditAnywhere, Config, Category = "Common", AdvancedDisplay, meta = (EditCondition = bImportSkeletalMesh))
    bool bImportAnimation;

    UPROPERTY(EditAnywhere, Config, Category = "Common")
    bool bImportMaterial;

    UPROPERTY(EditAnywhere, Config, Category = "Common", AdvancedDisplay, meta = (EditCondition = bImportMaterial))
    bool bImportTexture;
    
    UPROPERTY(EditAnywhere, Config, Category = "Common", AdvancedDisplay)
    bool bImportAllScene;
    
    /// mesh options
    UPROPERTY(EditAnywhere, Config, Category = "Mesh", meta = (ClampMin = 0.0001, ClampMax = 100000.0f))
    float MeshScaleRatio;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh")
    bool bApplyAbsoluteTransform;
    
    UPROPERTY(EditAnywhere, Config, Category = "Mesh")
    bool bGenerateLightmapUVs;
    
    UPROPERTY(EditAnywhere, Config, Category = "Mesh", AdvancedDisplay)
    bool bInvertNormal;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh", AdvancedDisplay)
    bool bUseMikkTSpace;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh", AdvancedDisplay)
    bool bRecomputeNormals;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh", AdvancedDisplay)
    bool bRecomputeTangents;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh", AdvancedDisplay)
    bool bRemoveDegenerates;
    
    UPROPERTY(EditAnywhere, Config, Category = "Mesh", AdvancedDisplay)
    bool bBuildAdjacencyBuffer;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh", AdvancedDisplay)
    bool bUseFullPrecisionUVs;
    
    UPROPERTY(EditAnywhere, Config, Category = "Mesh", AdvancedDisplay, meta = (EditCondition = bImportSkeletalMesh))
    bool bCreatePhysicsAsset;

    /// material options
    UPROPERTY(EditAnywhere, Config, Category = "Material", meta = (EditCondition = bImportMaterial))
    bool bUseMaterialInstance;
};

UCLASS(config = glTFForUE4Settings, defaultconfig)
class GLTFFORUE4_API UglTFImporterOptionsDetails : public UObject
{
    GENERATED_UCLASS_BODY()

public:
    /// common options
    UPROPERTY(EditAnywhere, Config, Category = "Common", meta = (ToolTip = "Construct the skeletal mesh when node has skin"))
    bool bImportSkeletalMesh;
    
    UPROPERTY(EditAnywhere, Config, Category = "Common", AdvancedDisplay, meta = (EditCondition = bImportSkeletalMesh))
    bool bImportAnimation;

    UPROPERTY(EditAnywhere, Config, Category = "Common")
    bool bImportMaterial;

    UPROPERTY(EditAnywhere, Config, Category = "Common", AdvancedDisplay, meta = (EditCondition = bImportMaterial))
    bool bImportTexture;
    
    UPROPERTY(EditAnywhere, Config, Category = "Common", AdvancedDisplay)
    bool bImportAllScene;
    
    UPROPERTY(EditAnywhere, Config, Category = "Common", meta = (ToolTip = "Construct the scene in the current level or new level"))
    bool bImportLevel;

    UPROPERTY(EditAnywhere, Config, Category = "Common", AdvancedDisplay, meta = (EditCondition = bImportLevel, AllowedClasses = "World", ToolTip = "Import a new level that create by the template"))
    FStringAssetReference ImportLevelTemplate;

    /// mesh options
    UPROPERTY(EditAnywhere, Config, Category = "Mesh", meta = (ClampMin = 0.0001, ClampMax = 100000.0f))
    float MeshScaleRatio;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh")
    bool bApplyAbsoluteTransform;
    
    UPROPERTY(EditAnywhere, Config, Category = "Mesh")
    bool bGenerateLightmapUVs;
    
    UPROPERTY(EditAnywhere, Config, Category = "Mesh", AdvancedDisplay)
    bool bInvertNormal;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh", AdvancedDisplay)
    bool bUseMikkTSpace;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh", AdvancedDisplay)
    bool bRecomputeNormals;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh", AdvancedDisplay)
    bool bRecomputeTangents;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh", AdvancedDisplay)
    bool bRemoveDegenerates;
    
    UPROPERTY(EditAnywhere, Config, Category = "Mesh", AdvancedDisplay)
    bool bBuildAdjacencyBuffer;

    UPROPERTY(EditAnywhere, Config, Category = "Mesh", AdvancedDisplay)
    bool bUseFullPrecisionUVs;
    
    UPROPERTY(EditAnywhere, Config, Category = "Mesh", AdvancedDisplay, meta = (EditCondition = bImportSkeletalMesh))
    bool bCreatePhysicsAsset;

    /// material options
    UPROPERTY(EditAnywhere, Config, Category = "Material", meta = (EditCondition = bImportMaterial))
    bool bUseMaterialInstance;

    void Get(FglTFImporterOptionsDetailsStored& OutDetailsStored) const;
    void Set(const FglTFImporterOptionsDetailsStored& InDetailsStored);
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
    FglTFImporterOptionsDetailsStored DetailsStored;

    UPROPERTY()
    UglTFImporterOptionsDetails* Details;
};
