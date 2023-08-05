/*  Copyright 2022 Pretendo Network contributors <pretendo.network>
    Copyright 2022 Ash Logan <ash@heyquark.com>
    Copyright 2019 Maschell

    Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
    granted, provided that the above copyright notice and this permission notice appear in all copies.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
    INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
    IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
    PERFORMANCE OF THIS SOFTWARE.
*/

#include <wups.h>
#include <utils/logger.h>
#include <coreinit/screen.h>
#include <mocha/mocha.h>

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("HaltFix");
WUPS_PLUGIN_DESCRIPTION("Reboots the Wii U upon a game crash");
WUPS_PLUGIN_VERSION("v1.0");
WUPS_PLUGIN_AUTHOR("shutterbug2000");
WUPS_PLUGIN_LICENSE("GPL");

extern "C" {
    void OSSendAppSwitchRequest(uint32_t rampid, void* args, uint32_t argsSize);
}

DECL_FUNCTION(void, PPCHalt) {
    OSSendAppSwitchRequest(5,0,0); //from what I can tell, staying in the crashed process causes the reboot to fail. So I switch to something else (HOME Menu overlay)
    OSScreenInit();
    OSScreenEnableEx(SCREEN_TV, 1);
    OSScreenEnableEx(SCREEN_DRC, 1);
    uint32_t tvSize = OSScreenGetBufferSizeEx(SCREEN_TV);
    OSScreenSetBufferEx(SCREEN_TV, (unsigned char*)0xf4000000);
    OSScreenSetBufferEx(SCREEN_DRC, (unsigned char*)0xf4000000+tvSize);
    OSScreenClearBufferEx(SCREEN_TV,0x0000ffff);
    OSScreenClearBufferEx(SCREEN_DRC,0x0000ffff);
    OSScreenPutFontEx(SCREEN_TV,0,0,"An error has occurred in the running game or application.\nYour Wii U console will now reboot.");
    OSScreenPutFontEx(SCREEN_DRC,0,0,"An error has occurred in the running game or application.\nYour Wii U console will now reboot.");
    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);
    OSSleepTicks(OSMillisecondsToTicks(5000));
    Mocha_IOSUCallSVC(0x74, 0, 0, 0);
    return;
}

INITIALIZE_PLUGIN() {
    WHBLogUdpInit();
    WHBLogCafeInit();
    DEBUG_FUNCTION_LINE("HaltFix loaded");

    auto res = Mocha_InitLibrary();

    if (res != MOCHA_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("Mocha init failed with code %d!", res);
        return;
    }
}

DEINITIALIZE_PLUGIN() {
    WHBLogUdpDeinit();
    WHBLogCafeDeinit();
    Mocha_DeInitLibrary();
}

ON_APPLICATION_START() {
}

ON_APPLICATION_ENDS() {
}

WUPS_MUST_REPLACE_FOR_PROCESS(PPCHalt, WUPS_LOADER_LIBRARY_COREINIT, PPCHalt, WUPS_FP_TARGET_PROCESS_GAME);