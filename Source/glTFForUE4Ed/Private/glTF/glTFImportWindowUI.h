#pragma once

#include "glTFImportWindowUI.generated.h"

UCLASS(config = EditorPerProjectUserSettings, HideCategories = Object, MinimalAPI)
class UglTFImportWindowUI : public UObject
{
    GENERATED_UCLASS_BODY()

public:
    /** True to import animations from the FBX File */
    UPROPERTY(EditAnywhere, config, Category = Animation, meta = (ImportType = "SkeletalMesh|Animation"))
    uint32 bImportAnimations : 1;

    /** Override for the name of the animation to import **/
    UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Animation, meta = (editcondition = "bImportAnimations", ImportType = "SkeletalMesh"))
    FString AnimationName;
};
