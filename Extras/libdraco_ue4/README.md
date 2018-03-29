# libdraco_ue4

Use the [Draco][] and make it as a third-party library in [UE4][].

[![draco status](https://img.shields.io/badge/draco-1%2E2%2E5-green.svg?style=flat)](https://github.com/google/draco)

[![Follow in twitter](https://img.shields.io/badge/follow-in%20twitter-blue.svg?style=flat)](https://twitter.com/C4gIo)
[![Join discord](https://img.shields.io/badge/chat-on%20discord-blue.svg?style=flat)](https://discord.gg/tyEjtQB)

[![pipeline status](https://gitlab.com/c4g/draco/libdraco_ue4/badges/master/pipeline.svg)](https://gitlab.com/c4g/draco/libdraco_ue4/commits/master)

[![Become a patreon](https://img.shields.io/badge/donation-become%20a%20patreon-ff69b4.svg?style=flat)](https://www.patreon.com/bePatron?u=7553208)
[![Patreon invite](https://img.shields.io/badge/donation-patreon%20invite-ff69b4.svg?style=flat)](https://patreon.com/invite/zpdxnv)

## Usage

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

## Roadmap

[![Visit in trello](https://img.shields.io/badge/visit-trello-blue.svg?style=flat)](https://trello.com/b/1yQyCz0D)

## Unreal Engine 4 Version

| UE4 | Status | Download |
|:---:|:------:|:--------:|
| [![Support UE4.10](https://img.shields.io/badge/ue-4%2E10-green.svg?style=flat)](#) | [![Build success](https://img.shields.io/badge/build-success-green.svg?style=flat)](#) | [![Download from itch.io](https://img.shields.io/badge/from-itch%2Eio-blue.svg?style=flat)](https://c4gio.itch.io/gltf-for-ue4) |
| [![Support UE4.11](https://img.shields.io/badge/ue-4%2E11-green.svg?style=flat)](#) | [![Build success](https://img.shields.io/badge/build-success-green.svg?style=flat)](#) | Same as UE4.10 |
| [![Support UE4.12](https://img.shields.io/badge/ue-4%2E12-green.svg?style=flat)](#) | [![Build success](https://img.shields.io/badge/build-success-green.svg?style=flat)](#) | Same as UE4.10 |
| [![Support UE4.13](https://img.shields.io/badge/ue-4%2E13-green.svg?style=flat)](#) | [![Build success](https://img.shields.io/badge/build-success-green.svg?style=flat)](#) | Same as UE4.10 |
| [![Support UE4.14](https://img.shields.io/badge/ue-4%2E14-green.svg?style=flat)](#) | [![Build success](https://img.shields.io/badge/build-success-green.svg?style=flat)](#) | Same as UE4.10 |
| [![Support UE4.15](https://img.shields.io/badge/ue-4%2E15-green.svg?style=flat)](#) | [![Build success](https://img.shields.io/badge/build-success-green.svg?style=flat)](#) | Same as UE4.10 |
| [![Support UE4.16](https://img.shields.io/badge/ue-4%2E16-green.svg?style=flat)](#) | [![Build success](https://img.shields.io/badge/build-success-green.svg?style=flat)](#) | [![Download from itch.io](https://img.shields.io/badge/from-itch%2Eio-blue.svg?style=flat)](https://c4gio.itch.io/gltf-for-ue4) |
| [![Support UE4.17](https://img.shields.io/badge/ue-4%2E17-green.svg?style=flat)](#) | [![Build success](https://img.shields.io/badge/build-success-green.svg?style=flat)](#) | Same as UE4.16 |
| [![Support UE4.18](https://img.shields.io/badge/ue-4%2E18-green.svg?style=flat)](#) | [![Build success](https://img.shields.io/badge/build-success-green.svg?style=flat)](#) | Same as UE4.16 |
| [![Support UE4.19](https://img.shields.io/badge/ue-4%2E19-green.svg?style=flat)](#) | [![Build success](https://img.shields.io/badge/build-success-green.svg?style=flat)](#) | Same as UE4.16 |

## License

[![The MIT License](https://img.shields.io/badge/license-MIT-blue.svg?style=flat)](https://github.com/code4game/libdraco_ue4/blob/master/LICENSE.md)

[Draco]:https://google.github.io/draco/
[UE4]:https://www.unrealengine.com/
[Unreal Engine 4]:https://www.unrealengine.com/
