#include "WS_Button.h"

#include "OneButton.h"
#include "WS_Audio.h"

#define BOOT_BUTTON_PIN 0

static OneButton bootButton(BOOT_BUTTON_PIN, true);

static void bootClick()
{
    printf("[BOOT] click: toggle built-in audio\r\n");
    Audio_ToggleMusic();
}

static void bootLongPressStart()
{
    printf("[BOOT] long press: record 5s and play\r\n");
    Audio_Record5sAndPlay();
}

void Button_Init()
{
    printf("[Button] init BOOT on GPIO%d\r\n", BOOT_BUTTON_PIN);
    bootButton.setPressMs(800);
    bootButton.attachClick(bootClick);
    bootButton.attachLongPressStart(bootLongPressStart);
}

void Button_Loop()
{
    bootButton.tick();
}
