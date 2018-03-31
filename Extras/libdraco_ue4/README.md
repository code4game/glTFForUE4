# libdraco_ue4

Use the [Draco][] and make it as a third-party library in [UE4][].

[![draco status](https://img.shields.io/badge/draco-1%2E2%2E5-green.svg?style=flat)](https://github.com/google/draco)

[![The MIT License](https://img.shields.io/badge/license-MIT-blue.svg?style=flat)](./blob/master/LICENSE.md)

[![Visit in trello](https://img.shields.io/badge/roadmap-trello-blue.svg?style=flat)](https://trello.com/b/1yQyCz0D)

[![Follow in twitter](https://img.shields.io/badge/follow-in%20twitter-blue.svg?style=flat)](https://twitter.com/C4gIo)
[![Join discord](https://img.shields.io/badge/chat-on%20discord-blue.svg?style=flat)](https://discord.gg/tyEjtQB)

[![pipeline status](https://gitlab.com/c4g/draco/libdraco_ue4/badges/master/pipeline.svg)](https://gitlab.com/c4g/draco/libdraco_ue4/commits/master)

[![Become a patreon](https://img.shields.io/badge/donation-become%20a%20patreon-ff69b4.svg?style=flat)](https://www.patreon.com/bePatron?u=7553208)
[![Patreon invite](https://img.shields.io/badge/donation-patreon%20invite-ff69b4.svg?style=flat)](https://patreon.com/invite/zpdxnv)

[![Support UE4.10](https://img.shields.io/badge/support-ue%204%2E10-green.svg?style=flat)](https://www.unrealengine.com/)
[![Support UE4.11](https://img.shields.io/badge/support-ue%204%2E11-green.svg?style=flat)](https://www.unrealengine.com/)
[![Support UE4.12](https://img.shields.io/badge/support-ue%204%2E12-green.svg?style=flat)](https://www.unrealengine.com/)
[![Support UE4.13](https://img.shields.io/badge/support-ue%204%2E13-green.svg?style=flat)](https://www.unrealengine.com/)
[![Support UE4.14](https://img.shields.io/badge/support-ue%204%2E14-green.svg?style=flat)](https://www.unrealengine.com/)
[![Support UE4.15](https://img.shields.io/badge/support-ue%204%2E15-green.svg?style=flat)](https://www.unrealengine.com/)
[![Support UE4.16](https://img.shields.io/badge/support-ue%204%2E16-green.svg?style=flat)](https://www.unrealengine.com/)
[![Support UE4.17](https://img.shields.io/badge/support-ue%204%2E17-green.svg?style=flat)](https://www.unrealengine.com/)
[![Support UE4.18](https://img.shields.io/badge/support-ue%204%2E18-green.svg?style=flat)](https://www.unrealengine.com/)
[![Support UE4.19](https://img.shields.io/badge/support-ue%204%2E19-green.svg?style=flat)](https://www.unrealengine.com/)

[![Windows 32-bit and 64-bit](https://img.shields.io/badge/platform-windows%2032bit%20and%2064bit-green.svg?style=flat)](#)
[![Linux](https://img.shields.io/badge/platform-linux-green.svg?style=flat)](#)
[![macOS](https://img.shields.io/badge/platform-macos-green.svg?style=flat)](#)
[![iOS](https://img.shields.io/badge/platform-ios-green.svg?style=flat)](#)

[![Download from itch.io](https://img.shields.io/badge/download-itch%2Eio-blue.svg?style=flat)](https://c4gio.itch.io/libdraco-ue4)

## Usage

### Copy to your project/plugin

First of all, download the archive [![Download from itch.io](https://img.shields.io/badge/from-itch%2Eio-blue.svg?style=flat)](https://c4gio.itch.io/libdraco-ue4).

Then, you just extra the archive in [your_project_root]/ThirdParty/ for a UE4 project, or extra the archive in [your_plugin_root]/Source/ThirdParty/ for a UE4 plugin.

### Add to the libdraco_ue4 as a module in your module

Just add the `libdraco_ue4` to the `PublicDependencyModuleNames` list in [your_module.Build.cs].

### Include the draco's header file

When include the draco's header file, you need write like this:

> Because it has a compiler conflict between the `draco::Status::Code::ERROR` and the macro `ERROR` in UE4 when compile.
>
> For example `#include "draco/compression/decode.h"`.

```cpp
#if defined(ERROR)
#define DRACO_MACRO_TEMP_ERROR      ERROR
#undef ERROR
#endif

#include "draco/compression/decode.h"

#if defined(DRACO_MACRO_TEMP_ERROR)
#define ERROR           DRACO_MACRO_TEMP_ERROR
#undef DRACO_MACRO_TEMP_ERROR
#endif
```

[Draco]:https://google.github.io/draco/
[UE4]:https://www.unrealengine.com/
[Unreal Engine 4]:https://www.unrealengine.com/
