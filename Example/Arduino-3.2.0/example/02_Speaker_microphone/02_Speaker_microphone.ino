#include "WS_Audio.h"
#include "WS_Button.h"

void setup()
{
    printf("09_SPEAKER_MICROPHONE audio demo starting...\r\n");
    Audio_Init();
    Button_Init();
    printf("BOOT click: play/pause built-in audio. BOOT long press: record 5s and play.\r\n");
}

void loop()
{
    // Keep OneButton responsive while audio work runs in a FreeRTOS task.
    Button_Loop();
    delay(10);
}
