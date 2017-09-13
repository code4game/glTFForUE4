#include "glTFForUE4EdPrivatePCH.h"
#include "glTFBinaryFactory.h"

#include "glTFImportUI.h"

#include "Engine/StaticMesh.h"
#include "Misc/Paths.h"

UglTFBinaryFactory::UglTFBinaryFactory(const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    SupportedClass = UStaticMesh::StaticClass();
    if (Formats.Num() > 0) Formats.Empty();
    Formats.Add(TEXT("glb;glTF 2.0 binary"));

    bCreateNew = false;
    bText = false;
    bEditorImport = true;
}

bool UglTFBinaryFactory::DoesSupportClass(UClass* InClass)
{
    //TODO:
    return (InClass == UStaticMesh::StaticClass());
}

bool UglTFBinaryFactory::FactoryCanImport(const FString& InFilename)
{
    return FPaths::GetExtension(InFilename).Equals(TEXT("glb"), ESearchCase::IgnoreCase);
}

UObject* UglTFBinaryFactory::FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, const TCHAR* InType, const uint8*& InBuffer, const uint8* InBufferEnd, FFeedbackContext* InWarn)
{
    //TODO:
    return nullptr;
}
