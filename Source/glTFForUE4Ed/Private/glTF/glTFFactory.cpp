#include "glTFForUE4EdPrivatePCH.h"
#include "glTFFactory.h"

#include "glTF/glTFImportWindow.h"
#include "glTF/glTFImportOptions.h"
#include "glTF/glTFImporter.h"

#include "libgltf/libgltf.h"

#include "Engine/StaticMesh.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "FglTFForUE4EdModule"

UglTFFactory::UglTFFactory(const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    SupportedClass = UStaticMesh::StaticClass();
    if (Formats.Num() > 0) Formats.Empty();
    Formats.Add(TEXT("gltf;glTF 2.0"));

    bCreateNew = false;
    bText = true;
    bEditorImport = true;
}

bool UglTFFactory::DoesSupportClass(UClass* InClass)
{
    //TODO:
    return (InClass == UStaticMesh::StaticClass());
}

bool UglTFFactory::FactoryCanImport(const FString& InFilePathInOS)
{
    return FPaths::GetExtension(InFilePathInOS).Equals(TEXT("gltf"), ESearchCase::IgnoreCase);
}

UObject* UglTFFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* InContext, const TCHAR* InType, const TCHAR*& InBuffer, const TCHAR* InBufferEnd, FFeedbackContext* InWarn)
{
    const FString& FilePathInOS = UFactory::GetCurrentFilename();
    if (!FPaths::GetBaseFilename(FilePathInOS).Equals(InName.ToString()))
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("It is different between current filename(%s) and name(%s)!!"), *FilePathInOS, *InName.ToString());
        return nullptr;
    }

    /// Open import window, allow to configure some options
    bool bCancel = false;
    TSharedPtr<FglTFImportOptions> glTFImportOptions = SglTFImportWindow::Open(FilePathInOS, InParent->GetPathName(), bCancel);
    if (bCancel)
    {
        UE_LOG(LogglTFForUE4Ed, Display, TEXT("Cancel to import the file - %s"), *FilePathInOS);
        return nullptr;
    }
    if (!glTFImportOptions.IsValid())
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Failed to open import window"));
        return nullptr;
    }

    std::shared_ptr<libgltf::SGlTF> GlTF;
    std::wstring GlTFString = InBuffer;
    bool bIsSuccessToParse = (GlTF << GlTFString);
    if (!bIsSuccessToParse)
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Failed to parse the gltf file %s"), *InName.ToString());
        return nullptr;
    }

    return FglTFImporter::Get().Create(glTFImportOptions, GlTF, InClass, InParent, InWarn);
}

#undef LOCTEXT_NAMESPACE
