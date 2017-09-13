#include "glTFForUE4EdPrivatePCH.h"
#include "glTFFactory.h"

#include "glTFImportUI.h"

#include "libgltf/libgltf.h"

#include "Engine/StaticMesh.h"
#include "Misc/Paths.h"

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

bool UglTFFactory::FactoryCanImport(const FString& InFilename)
{
    return FPaths::GetExtension(InFilename).Equals(TEXT("gltf"), ESearchCase::IgnoreCase);
}

UObject* UglTFFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* InContext, const TCHAR* InType, const TCHAR*& InBuffer, const TCHAR* InBufferEnd, FFeedbackContext* InWarn)
{
    std::shared_ptr<libgltf::SGlTF> GlTF;
    std::wstring GlTFString = InBuffer;
    if (!(GlTF << GlTFString))
    {
        return nullptr;
    }
    for (const std::shared_ptr<libgltf::SBuffer>& buffer : GlTF->buffers)
    {
        int32 dd = 0;
        buffer->name;
    }
    //
    return nullptr;
}
