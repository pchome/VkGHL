# VkGHL

Vulkan _(Game Halt, Green Hog, ... whatever)?_ Layer.<br/>
_Force anisotropy, mip LOD Bias, VSYNC and limit frame rate for Vulkan-driven games._

## Settings (environment variables)

* **FPS** - limit frame time (`>0`)
* **AF** - Anisotropy (`0` - Off, `1..16` - On)
* **MIPLODBIAS** - Mip Map LOD Bias (`-16..16` (aka `sharpen..blurry`, `-1` is ok in general))<br/>
  _Consider to disable AA/AF when setting this._
* **RETRO** - "retro" filter (`0` - Off, `1` - On)
* **VSYNC**<br/>
  `0` - Off<br/>
  `1` - Mailbox (uncapped framerate)<br/>
  `2` - On<br/>
  `3` - Adaptive

## Build

#### Prepare

1. `git clone https://github.com/LunarG/VulkanTools`
2. `cd VulkanTools`
3. [optional] `for d in assistant_layer demo_layer starter_layer; do rm -r layer_factory/$d; done`
4. `git submodule add -- https://github.com/pchome/VkGHL layer_factory/VkGHL`

#### Build

1. `mkdir -p build_layer`
2. `cd build_layer`
3. NOTE: full path to Vulkan header files (may fix `"vk_loader_platform.h"` compile-time errors)
```
cmake \
-DCMAKE_CXX_FLAGS="-I/usr/include/vulkan" \
-DCMAKE_BUILD_TYPE=Release \
-DCMAKE_INSTALL_PREFIX=/tmp/VkGHL \
-DBUILD_TESTS=OFF \
-DBUILD_LAYERSVT=OFF \
-DBUILD_VKTRACE=OFF \
-DBUILD_VIA=OFF \
-DBUILD_LAYERMGR=OFF \
-DBUILD_VKTRACE_REPLAY=OFF \
-DVULKAN_HEADERS_INSTALL_DIR=usr/ \
-DVULKAN_LOADER_INSTALL_DIR=usr/ \
-DVULKAN_VALIDATIONLAYERS_INSTALL_DIR=usr/ \
..
```
4. `make`
5. Repeat steps 1..4 for 32bit build<br>
    (e.g. use `build_layer32` name and `-DCMAKE_CXX_FLAGS="-m32 -I/usr/include/vulkan"`)

#### Try
`cd layers`

```
MIPLODBIAS=-1 \
AF=16 \
RETRO=1 \
VSYNC=0 \
FPS=150.55 \
DXVK_HUD=1 \
VK_LAYER_PATH=. \
VK_INSTANCE_LAYERS="VK_LAYER_LUNARG_VkGHL" \
wine d3d11-triangle.exe
```

#### Suitable installation path examples
In such cases you can ommit `VK_LAYER_PATH=` environment variable.
Install `*.so` files into directory visible via `LD_LIBRARY_PATH=`.

System
```
/usr/lib/libVkLayer_VkGHL.so
/usr/lib64/libVkLayer_VkGHL.so
/usr/share/vulkan/explicit_layer.d/VkLayer_VkGHL.json
```

User (add `LD_LIBRARY_PATH="${HOME}/.local/lib64:${HOME}/.local/lib"` to your env)
```
~/.local/lib/libVkLayer_VkGHL.so
~/.local/lib64/libVkLayer_VkGHL.so
~/.local/share/vulkan/explicit_layer.d/VkLayer_VkGHL.json
```

## Useful tools
[vkconfig](https://github.com/LunarG/VulkanTools/blob/master/vkconfig/vkconfig.md)

> The Vulkan Configurator is a graphical application that allows a user to specify which layers will be loaded by Vulkan applications at runtime. It provides an alternative to setting layers through environment variables or an application's layer selection. In addition, it allows using layers from non-standard locations, selecting the ordering for implicit layers, and specifying settings for layers that Vulkan Configurator supports.

## Note

Currently mimics [libstrangle](https://gitlab.com/torkel104/libstrangle) behaviour and env vars (UPPERCASE ones).
This may change in future.
