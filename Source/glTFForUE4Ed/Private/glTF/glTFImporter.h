#pragma once

#include <vector>
#include <memory>

namespace libgltf
{
    struct SGlTF;
    struct SGlTFId;
    struct SMesh;
    struct SNode;
}

class FglTFImporter
{
public:
    static const FglTFImporter& Get();

private:
    FglTFImporter();
    virtual ~FglTFImporter();

public:
    UObject* CreateMesh(const TWeakPtr<struct FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, UClass* InClass, UObject* InParent) const;

private:
    UStaticMesh* CreateStaticMesh(const TWeakPtr<struct FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SMesh>& InMesh, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const class FBufferFiles& InBufferFiles) const;
    bool CreateStaticMesh(const TWeakPtr<struct FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SNode>& InNode, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const class FBufferFiles& InBufferFiles, TArray<UStaticMesh*>& OutStaticMeshes) const;
    bool CreateStaticMesh(const TWeakPtr<struct FglTFImportOptions>& InglTFImportOptions, const std::vector<std::shared_ptr<libgltf::SGlTFId>>& InNodeIndices, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const class FBufferFiles& InBufferFiles, TArray<UStaticMesh*>& OutStaticMeshes) const;

public:
    static bool GetTriangleIndices(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const class FBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<uint32>& OutTriangleIndices);
    static bool GetVertexPositions(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const class FBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<FVector>& OutVertexPositions);
    static bool GetVertexNormals(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const class FBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<FVector>& OutVertexNormals);
    static bool GetVertexTangents(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const class FBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<FVector4>& OutVertexTangents);
    static bool GetVertexTexcoords(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const class FBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<FVector2D>& OutVertexTexcoords);
};
