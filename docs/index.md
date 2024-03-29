# **glTF** for **UE4**

This is a **[glTF][glTF]** importer for **[UE4/5][UE4/5](Unreal Engine 4 & 5)**.

## Features

* Load and parse the [glTF][glTF] 2.0 file
    * **json**
    * **json-embedded**
    * **binary**
* Import the **static mesh** in UE4/5 editor
    * Reimport the **static mesh**
    * Support the **morph targets**
    * Spawn all **static meshes** as `AStaticMeshActor` in a level
* Import the **skeletal mesh** in UE4/5 editor
    * Reimport the **skeletal mesh**
    * Support the **morph targets**
    * Import the **skeleton animation**
    * Create the **physics asset**
    * Spawn all **skeletal meshes** as `ASkeletalMeshActor` in a level
* Imports the material(`PBR`) in UE4/5 editor
* Imports the texture in UE4/5 editor
* Allow to transform from local to world
* Extensions
    * **`KHR_draco_mesh_compression`**
    * **`KHR_materials_pbrSpecularGlossiness`**

## More powerful features in the future

You can [visit all in github's milestones][Github Milestones].

## License

MIT License

[glTF]: https://github.com/KhronosGroup/glTF "glTF"
[UE4/5]: https://unrealengine.com "Unreal Engine 4 & 5"
[Github Milestones]: https://github.com/code4game/glTFForUE4/milestones "Github Milestones"
