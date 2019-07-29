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
5. `cd layers`

#### Try

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

## Note

Currently mimics [libstrangle](https://gitlab.com/torkel104/libstrangle) behaviour and env vars (UPPERCASE ones).
This may change in future.
