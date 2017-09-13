#include "glTFForUE4EdPrivatePCH.h"
#include "glTFFactory.h"

#include "glTFImportUI.h"

#include "libgltf/libgltf.h"

#include "Engine/StaticMesh.h"
#include "Misc/Paths.h"

UglTFFactory::UglTFFactory(const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
    , glTFImportUI(nullptr)
{
    SupportedClass = UStaticMesh::StaticClass();
    Formats.Add(TEXT("gltf;glTF meshes and animations"));

    bCreateNew = false;
    bText = true;
    bEditorImport = true;
    //
}

bool UglTFFactory::DoesSupportClass(UClass* InClass)
{
    return (InClass == UStaticMesh::StaticClass());
}

bool UglTFFactory::FactoryCanImport(const FString& InFilename)
{
    const FString Extension = FPaths::GetExtension(InFilename);
    return Extension.Equals(TEXT("gltf"), ESearchCase::IgnoreCase)
        || Extension.Equals(TEXT("glb"), ESearchCase::IgnoreCase);
}

UObject* UglTFFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* InContext, const TCHAR* InType, const TCHAR*& InBuffer, const TCHAR* InBufferEnd, FFeedbackContext* InWarn)
{
    std::shared_ptr<libgltf::SGlTF> GlTF;
    std::wstring GlTFString = InBuffer;
    if (!(GlTF << GlTFString))
    {
        return nullptr;
    }
    //
    return nullptr;
}
