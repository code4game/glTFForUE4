#pragma once

#include <memory>

namespace libgltf
{
    struct SGlTF;
}

class FglTFImporter
{
public:
    static const FglTFImporter& Get();

private:
    FglTFImporter();
    virtual ~FglTFImporter();

public:
    UObject* CreateMesh(const TSharedPtr<struct FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, UClass* InClass, UObject* InParent) const;
};
