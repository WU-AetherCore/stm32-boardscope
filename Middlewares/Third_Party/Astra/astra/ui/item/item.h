//
// Created by Fir on 2024/4/14 014.
//
#pragma once
#ifndef ASTRA_CORE_SRC_ASTRA_UI_ITEM_ITEM_H_
#define ASTRA_CORE_SRC_ASTRA_UI_ITEM_ITEM_H_

#include <cmath>
#include "../../../hal/hal.h"
#include "../../../astra/config/config.h"

namespace astra {

class Item {
protected:
  // V1.2：共享配置引用，避免每个菜单复制约 200 字节配置。
  // 所有 Item 在 HAL 注入后创建；配置与 HAL 存活至复位。
  sys::config& systemConfig = HAL::getSystemConfig();
  config& astraConfig = getUIConfig();
  inline void updateConfig() {} // 共享引用始终读取最新配置。

};

class Animation {
public:
  inline static uint32_t frameDelta=16,lastFrame=0;
  static void beginFrame(uint32_t now) { frameDelta=lastFrame?now-lastFrame:16;lastFrame=now;if(frameDelta>40)frameDelta=40;if(!frameDelta)frameDelta=1; }
  static void entry();
  static void exit();
  static void blur();
  static void move(float *_pos, float _posTrg, float _speed);
};

inline void Animation::entry() { }

//todo 未实现功能
inline void Animation::exit() {
  static unsigned char fadeFlag = 1;
  static uint16_t bufferLen = 8 * HAL::getBufferTileHeight() * HAL::getBufferTileWidth();
  auto *bufferPointer = (unsigned char *) HAL::getCanvasBuffer();

  HAL::delay(getUIConfig().fadeAnimationSpeed);

  if (getUIConfig().lightMode)
    switch (fadeFlag) {
      case 1:
        for (uint16_t i = 0; i < bufferLen; ++i) if (i % 2 != 0) bufferPointer[i] = bufferPointer[i] & 0xAA;
        break;
      case 2:
        for (uint16_t i = 0; i < bufferLen; ++i) if (i % 2 != 0) bufferPointer[i] = bufferPointer[i] & 0x00;
        break;
      case 3:
        for (uint16_t i = 0; i < bufferLen; ++i) if (i % 2 == 0) bufferPointer[i] = bufferPointer[i] & 0x55;
        break;
      case 4:
        for (uint16_t i = 0; i < bufferLen; ++i) if (i % 2 == 0) bufferPointer[i] = bufferPointer[i] & 0x00;
        break;
      default:
        //放动画结束退出函数的代码
        fadeFlag = 0;
        break;
    }
  else
    switch (fadeFlag) {
      case 1:
        for (uint16_t i = 0; i < bufferLen; ++i) if (i % 2 != 0) bufferPointer[i] = bufferPointer[i] | 0xAA;
        break;
      case 2:
        for (uint16_t i = 0; i < bufferLen; ++i) if (i % 2 != 0) bufferPointer[i] = bufferPointer[i] | 0x00;
        break;
      case 3:
        for (uint16_t i = 0; i < bufferLen; ++i) if (i % 2 == 0) bufferPointer[i] = bufferPointer[i] | 0x55;
        break;
      case 4:
        for (uint16_t i = 0; i < bufferLen; ++i) if (i % 2 == 0) bufferPointer[i] = bufferPointer[i] | 0x00;
        break;
      default:
        fadeFlag = 0;
        break;
    }
  fadeFlag++;
}

inline void Animation::blur() {
  static uint16_t bufferLen = 8 * HAL::getBufferTileHeight() * HAL::getBufferTileWidth();
  static auto *bufferPointer = (unsigned char *) HAL::getCanvasBuffer();

  for (uint16_t i = 0; i < bufferLen; ++i) bufferPointer[i] = bufferPointer[i] & (i % 2 == 0 ? 0x55 : 0xAA);
}

inline void Animation::move(float *_pos, float _posTrg, float _speed) {
  if (*_pos != _posTrg) {
    if (std::fabs(*_pos - _posTrg) <= 1.0f) *_pos = _posTrg;
    else { float tau=(100-_speed)*8;float alpha=frameDelta/(tau+frameDelta);*_pos+=(_posTrg-*_pos)*alpha; }
  }
}
}

#endif //ASTRA_CORE_SRC_ASTRA_UI_ITEM_ITEM_H_
