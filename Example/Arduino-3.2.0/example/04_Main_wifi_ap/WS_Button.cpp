#include "WS_Button.h"

#include "OneButton.h"
#include "WS_Audio.h"

#define BOOT_BUTTON_PIN 0

static OneButton bootButton(BOOT_BUTTON_PIN, true);

static void bootClick()
{
  Audio_ToggleMusic();
}

static void bootLongPressStart()
{
  Audio_Record5sAndPlay();
}

void Button_Init()
{
  bootButton.setPressMs(800);
  bootButton.attachClick(bootClick);
  bootButton.attachLongPressStart(bootLongPressStart);
}

void Button_Loop()
{
  bootButton.tick();
}
