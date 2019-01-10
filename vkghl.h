#pragma once

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
    isDisabled = !testSettings();
    if (isDisabled)
      std::fprintf(stderr, "VkGHL: disabled\n");
  }

  VkResult PreCallQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo) override;
  VkResult PostCallQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo) override;
  VkResult PreCallCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                     VkSwapchainKHR *pSwapchain) override;
  VkResult PreCallCreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                VkSampler *pSampler) override;

 private:
  VkPresentModeKHR getVSync();

  TimeDiff getFrameTime();
  float    getLodBias();
  float    getAF();
  bool     getRetro();

  inline void limiter();
  const bool  testSettings();

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

VkResult VkGHL::PreCallQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo) {
  if (isDisabled)
    return VK_SUCCESS;
  frameStart = Clock::now();
  return VK_SUCCESS;
}

VkResult VkGHL::PostCallQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo) {
  if (isDisabled)
    return VK_SUCCESS;
  limiter();
  frameEnd = Clock::now();
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
    const_cast<VkBool32 &>(pCreateInfo->anisotropyEnable) = 1;
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
    std::fprintf(stderr, "VkGHL: FPS limit: %s fps\n", fpsLimit);
    double fps = std::stod(fpsLimit);
    if (fps != 0.0)
      return TimeDiff(long(1000000000.0f / fps));
  }
  return TimeDiff(0);
}

VkPresentModeKHR VkGHL::getVSync() {
  const char *vsyncType = std::getenv("VSYNC");
  if (vsyncType != nullptr && !std::string(vsyncType).empty()) {
    std::fprintf(stderr, "VkGHL: VSync: %s\n", vsyncType);
    int8_t vs = std::stoi(vsyncType);
    if (vs >= 0)
      return static_cast<VkPresentModeKHR>(vs);
  }
  return VkPresentModeKHR::VK_PRESENT_MODE_MAX_ENUM_KHR;
}

float VkGHL::getLodBias() {
  const char *lodBias = std::getenv("MIPLODBIAS");
  if (lodBias != nullptr && !std::string(lodBias).empty()) {
    std::fprintf(stderr, "VkGHL: mipLODBias: %s\n", lodBias);
    float mip = std::stof(lodBias);
    if (mip >= -16.0f && mip <= 15.99f)
      return mip;
  }
  return 666;
}

float VkGHL::getAF() {
  const char *aF = std::getenv("AF");
  if (aF != nullptr && !std::string(aF).empty()) {
    std::fprintf(stderr, "VkGHL: AF: %s\n", aF);
    float af = std::stof(aF);
    if (af >= 1 && af <= 16)
      return af;
  }
  return 666;
}

bool VkGHL::getRetro() {
  const char *retro = std::getenv("RETRO");
  if (retro != nullptr && !std::string(retro).empty()) {
    std::fprintf(stderr, "VkGHL: RETRO: %s\n", retro);
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
inline bool VkGHL::isValidAF() { return AF >= 1 && AF <= 16; }
inline bool VkGHL::isValidRetro() { return retroFilter; }

const bool VkGHL::testSettings() { return isValidFPS() || isValidVSync() || isValidBias() || isValidAF() || isValidRetro(); }

VkGHL vkghl;
