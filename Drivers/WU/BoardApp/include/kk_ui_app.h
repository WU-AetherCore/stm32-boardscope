#ifndef KK_UI_APP_H
#define KK_UI_APP_H

#include "kk_ui.h"

#include <stdint.h>

enum
{
    KK_UI_EVENT_BRIGHTNESS_CHANGED = 1,
    KK_UI_EVENT_SWITCH_CHANGED,
    KK_UI_EVENT_INTERVAL_CHANGED,
    KK_UI_EVENT_RESET_CONFIRMED,
    KK_UI_EVENT_RESET_CANCELLED,
    KK_UI_EVENT_SHOW_MESSAGE,
    KK_UI_EVENT_MESSAGE_ACKNOWLEDGED,
    KK_UI_EVENT_ADVANCED_VISIBILITY_CHANGED,
    KK_UI_EVENT_DEMO_LOCK_CHANGED,
    KK_UI_EVENT_ADVANCED_ACTION,
    KK_UI_EVENT_SHOW_TOAST
};

const KK_UI_App *KK_UI_AppGet(void);
/* Call after OLED_Init(), before KK_UI_Init(); resets application state only. */
void KK_UI_AppReset(uint32_t now_ms);
void KK_UI_AppProcessEvents(void);
void KK_UI_AppUpdate(uint32_t now_ms);

#ifdef KK_OLED_TEST
uint16_t KK_UI_AppTestWaveProgress(void);
bool KK_UI_AppTestWaveManual(void);
uint16_t KK_UI_AppTestWavePhase(void);
const char *KK_UI_AppTestWaveFps(void);
uint32_t KK_UI_AppTestWaveEnteredAt(void);
#endif

#endif
