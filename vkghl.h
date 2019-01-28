#pragma once

#include <iostream>
#include <thread>

#ifdef __clangd__
// For IDE
#include "layer_factory.h"
#include "layer_factory.cpp"
#endif

class VkGHL : public layer_factory {
  using Clock     = std::chrono::steady_clock;
  using TimeDiff  = std::chrono::nanoseconds;
  using TimePoint = typename Clock::time_point;

 public:
  VkGHL()
      : targetFrameTime(getFrameTime()),
        vSync(getVSync()),
        mipLODBias(getLodBias()),
        AF(getAF()),
        retroFilter(getRetro()),
        frameOverhead(0),
        frameStart(Clock::now()),
        frameEnd(Clock::now()) {
    layer_name = "VkGHL";
    isDisabled = !testSettings();
    if (isDisabled)
      std::cout << "VkGHL: Disabled\n";
  }

  VkResult PostCallQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo) override;
  VkResult PreCallCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                     VkSwapchainKHR *pSwapchain) override;
  VkResult PreCallCreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                VkSampler *pSampler) override;
  VkResult PostCallGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pPresentModeCount,
                                                           VkPresentModeKHR *pPresentModes) override;

 private:
  VkPresentModeKHR getVSync();

  TimeDiff getFrameTime();
  float    getLodBias();
  float    getAF();
  bool     getRetro();

  inline void limiter();
  const bool  testSettings();
  const char *getPresentModeName(VkPresentModeKHR mode);

  inline bool isValidFPS();
  inline bool isValidVSync();
  inline bool isValidBias();
  inline bool isValidAF();
  inline bool isValidRetro();

  VkPresentModeKHR vSync;

  const TimeDiff targetFrameTime;

  float mipLODBias;
  float AF;
  bool  retroFilter;

  TimeDiff  frameOverhead;
  TimePoint frameStart;
  TimePoint frameEnd;

  bool isDisabled;
};

VkResult VkGHL::PostCallQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo) {
  if (isDisabled)
    return VK_SUCCESS;
  frameStart = Clock::now();
  limiter();
  frameEnd = Clock::now();
  return VK_SUCCESS;
}

VkResult VkGHL::PostCallGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pPresentModeCount,
                                                                VkPresentModeKHR *pPresentModes) {
  if (pPresentModes != nullptr) {
    std::vector<VkPresentModeKHR> present_modes;
    present_modes.insert(present_modes.begin(), pPresentModes, pPresentModes + *pPresentModeCount);
    std::cout << "VkGHL: Present modes cnt: " << *pPresentModeCount << "\n";
    for (auto present_mode : present_modes)
      std::cout << "VkGHL:     " << getPresentModeName(present_mode) << ": " << present_mode << "\n";
  }
  return VK_SUCCESS;
}

VkResult VkGHL::PreCallCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                          VkSwapchainKHR *pSwapchain) {
  if (isDisabled)
    return VK_SUCCESS;
  if (isValidVSync())
    const_cast<VkPresentModeKHR &>(pCreateInfo->presentMode) = vSync;
  return VK_SUCCESS;
}

VkResult VkGHL::PreCallCreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                     VkSampler *pSampler) {
  if (isDisabled)
    return VK_SUCCESS;

  if (isValidBias())
    const_cast<float &>(pCreateInfo->mipLodBias) = mipLODBias;

  if (isValidAF()) {
    const_cast<VkBool32 &>(pCreateInfo->anisotropyEnable) = !(AF == 0);
    const_cast<float &>(pCreateInfo->maxAnisotropy)       = AF;
  }
  if (isValidRetro()) {
    const_cast<VkFilter &>(pCreateInfo->magFilter)             = VkFilter::VK_FILTER_NEAREST;
    const_cast<VkFilter &>(pCreateInfo->minFilter)             = VkFilter::VK_FILTER_NEAREST;
    const_cast<VkSamplerMipmapMode &>(pCreateInfo->mipmapMode) = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
  }
  return VK_SUCCESS;
}

std::chrono::nanoseconds VkGHL::getFrameTime() {
  const char *fpsLimit = std::getenv("FPS");
  if (fpsLimit != nullptr && !std::string(fpsLimit).empty()) {
    std::cout << "VkGHL: FPS limit: " << fpsLimit << " fps\n";
    double fps = std::stod(fpsLimit);
    if (fps != 0.0)
      return TimeDiff(uint64_t(1000000000.0f / fps));
  }
  return TimeDiff(0);
}

VkPresentModeKHR VkGHL::getVSync() {
  const char *vsyncType = std::getenv("VSYNC");
  if (vsyncType != nullptr && !std::string(vsyncType).empty()) {
    std::cout << "VkGHL: VSync: " << vsyncType << "\n";
    int8_t vs = std::stoi(vsyncType);
    if (vs >= 0)
      return static_cast<VkPresentModeKHR>(vs);
  }
  return VkPresentModeKHR::VK_PRESENT_MODE_MAX_ENUM_KHR;
}

float VkGHL::getLodBias() {
  const char *lodBias = std::getenv("MIPLODBIAS");
  if (lodBias != nullptr && !std::string(lodBias).empty()) {
    std::cout << "VkGHL: mipLODBias: " << lodBias << "\n";
    float mip = std::stof(lodBias);
    if (mip >= -16.0f && mip <= 15.99f)
      return mip;
  }
  return 666;
}

float VkGHL::getAF() {
  const char *aF = std::getenv("AF");
  if (aF != nullptr && !std::string(aF).empty()) {
    std::cout << "VkGHL: AF: " << aF << "\n";
    float af = std::stof(aF);
    if (af >= 0 && af <= 16)
      return af;
  }
  return 666;
}

bool VkGHL::getRetro() {
  const char *retro = std::getenv("RETRO");
  if (retro != nullptr && !std::string(retro).empty()) {
    std::cout << "VkGHL: RETRO: " << retro << "\n";
    return !(std::string(retro) == "0");
  }
  return false;
}

inline void VkGHL::limiter() {
  TimeDiff sleepTime = targetFrameTime - (Clock::now() - frameEnd);
  if (sleepTime > frameOverhead) {
    TimeDiff adjustedSleepTime = sleepTime - frameOverhead;
    std::this_thread::sleep_for(TimeDiff(adjustedSleepTime));
    frameOverhead = ((Clock::now() - frameStart) - adjustedSleepTime + frameOverhead * 99) / 100;
  }
}

inline bool VkGHL::isValidVSync() {
  return vSync >= VkPresentModeKHR::VK_PRESENT_MODE_BEGIN_RANGE_KHR && vSync <= VkPresentModeKHR::VK_PRESENT_MODE_END_RANGE_KHR;
}
inline bool VkGHL::isValidFPS() { return targetFrameTime > TimeDiff(0); }
inline bool VkGHL::isValidBias() { return mipLODBias >= -16.0f && mipLODBias <= 15.99f; }
inline bool VkGHL::isValidAF() { return AF == 0 || (AF >= 1 && AF <= 16); }
inline bool VkGHL::isValidRetro() { return retroFilter; }

const bool VkGHL::testSettings() { return isValidFPS() || isValidVSync() || isValidBias() || isValidAF() || isValidRetro(); }

inline const char *VkGHL::getPresentModeName(VkPresentModeKHR mode) {
  std::unordered_map<VkPresentModeKHR, const char *> modes;
  modes = {{VK_PRESENT_MODE_FIFO_RELAXED_KHR, "VK_PRESENT_MODE_FIFO_RELAXED_KHR"},
           {VK_PRESENT_MODE_IMMEDIATE_KHR, "VK_PRESENT_MODE_IMMEDIATE_KHR"},
           {VK_PRESENT_MODE_MAILBOX_KHR, "VK_PRESENT_MODE_MAILBOX_KHR"},
           {VK_PRESENT_MODE_FIFO_KHR, "VK_PRESENT_MODE_FIFO_KHR"}};
  return modes[mode];
}

VkGHL vkghl;
