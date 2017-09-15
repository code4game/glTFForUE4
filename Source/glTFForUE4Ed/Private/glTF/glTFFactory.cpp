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
    , CurrentFilename(TEXT(""))
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

bool UglTFFactory::FactoryCanImport(const FString& InFilename)
{
    //HACK: Store the filename, but it will be used in another function
    CurrentFilename = InFilename;
    return FPaths::GetExtension(InFilename).Equals(TEXT("gltf"), ESearchCase::IgnoreCase);
}

UObject* UglTFFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* InContext, const TCHAR* InType, const TCHAR*& InBuffer, const TCHAR* InBufferEnd, FFeedbackContext* InWarn)
{
    //HACK: Check the filename that was stored by another function
    if (!FPaths::GetBaseFilename(CurrentFilename).Equals(InName.ToString()))
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("It is different between current filename(%s) and name(%s)!!"), *CurrentFilename, *InName.ToString());
        return nullptr;
    }

    /// Open import window, allow to configure some options
    TSharedPtr<FglTFImportOptions> glTFImportOptions = SglTFImportWindow::Open(CurrentFilename, InParent->GetPathName());
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

    return FglTFImporter::Get().CreateMesh(glTFImportOptions, GlTF, InClass, InParent);
}

#undef LOCTEXT_NAMESPACE
