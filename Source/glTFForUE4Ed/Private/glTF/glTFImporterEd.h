// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "glTF/glTFImporter.h"
#include "glTF/glTFImporterOptions.h"
#include <EditorFramework/AssetImportData.h>
#include "glTFImporterEd.generated.h"

class FglTFImporterEd : public FglTFImporter
{
    typedef FglTFImporter Super;

public:
    struct FglTFMaterialInfo
    {
        explicit FglTFMaterialInfo(int32 InId, FString InPrimitiveName);

        int32 Id;
        FString PrimitiveName;
    };

public:
    static TSharedPtr<FglTFImporterEd> Get(class UFactory* InFactory, UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext);

public:
    FglTFImporterEd();
    virtual ~FglTFImporterEd();

public:
    virtual UObject* Create(const TWeakPtr<struct FglTFImporterOptions>& InglTFImporterOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBuffers& InglTFBuffers) const override;

protected:
    class UFactory* InputFactory;

public:
    static bool SetAssetImportData(UObject* InObject, const FglTFImporterOptions& InglTFImporterOptions);
    static UAssetImportData* GetAssetImportData(UObject* InObject);
    static void UpdateAssetImportData(UObject* InObject, const FString& InFilePathInOS);
    static void UpdateAssetImportData(UObject* InObject, const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions);
};

UCLASS()
class UglTFImporterEdData : public UAssetImportData
{
    GENERATED_UCLASS_BODY()

public:
    UPROPERTY(VisibleAnywhere, Category = glTFForUE4Ed)
    FglTFImporterOptions glTFImporterOptions;
};
