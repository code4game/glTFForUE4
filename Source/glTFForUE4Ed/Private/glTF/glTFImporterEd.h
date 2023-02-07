// Copyright(c) 2016 - 2023 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "glTF/glTFImporter.h"
#include "glTF/glTFImporterOptions.h"
#include <EditorFramework/AssetImportData.h>
#include "glTFImporterEd.generated.h"

class FglTFImporterEd : public FglTFImporter
{
    typedef FglTFImporter Super;

  public:
    static TSharedPtr<FglTFImporterEd> Get(class UFactory*         InFactory, //
                                           UObject*                InParent,
                                           FName                   InName,
                                           EObjectFlags            InFlags,
                                           class FFeedbackContext* InFeedbackContext);

  public:
    FglTFImporterEd();
    virtual ~FglTFImporterEd();

  public:
    virtual UObject* Create(const TWeakPtr<struct FglTFImporterOptions>& InglTFImporterOptions,
                            const std::shared_ptr<libgltf::SGlTF>&       InGlTF,
                            const FglTFBuffers&                          InglTFBuffers,
                            const glTFForUE4::FFeedbackTaskWrapper&      InFeedbackTaskWrapper) const override;

  private:
    UObject* CreateNodes(const TWeakPtr<struct FglTFImporterOptions>&          InglTFImporterOptions,
                         const std::shared_ptr<libgltf::SGlTF>&                InGlTF,
                         const std::vector<std::shared_ptr<libgltf::SGlTFId>>& InNodeIdPtrs,
                         const FglTFBuffers&                                   InglTFBuffers,
                         struct FglTFImporterCollection&                       InOutglTFImporterCollection) const;
    UObject* CreateNode(const TWeakPtr<struct FglTFImporterOptions>& InglTFImporterOptions,
                        const std::shared_ptr<libgltf::SGlTF>&       InGlTF,
                        const std::shared_ptr<libgltf::SGlTFId>&     InNodeIdPtr,
                        const FglTFBuffers&                          InglTFBuffers,
                        struct FglTFImporterCollection&              InOutglTFImporterCollection) const;

  protected:
    class UFactory* InputFactory;

  public:
    static bool              SetAssetImportData(UObject* InObject, const FglTFImporterOptions& InglTFImporterOptions);
    static UAssetImportData* GetAssetImportData(UObject* InObject);
    static void              UpdateAssetImportData(UObject* InObject, const FString& InFilePathInOS);
    static void              UpdateAssetImportData(UObject* InObject, const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions);
};

UCLASS()
class UglTFImporterEdData : public UAssetImportData
{
    GENERATED_UCLASS_BODY()

  public:
    UPROPERTY(VisibleAnywhere, Category = glTFForUE4Ed)
    FglTFImporterOptions glTFImporterOptions;
};
