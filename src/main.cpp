/*  Copyright 2023 shutterbug2000
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
#include <coreinit/ios.h>

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("HaltFix");
WUPS_PLUGIN_DESCRIPTION("Reboots the Wii U upon a game crash");
WUPS_PLUGIN_VERSION("v1.0");
WUPS_PLUGIN_AUTHOR("shutterbug2000");
WUPS_PLUGIN_LICENSE("ISC");

extern "C" {
    void OSSendAppSwitchRequest(uint32_t rampid, void* args, uint32_t argsSize);
}

#define IOCTL_SVC            0x02

#define ALIGN(align)                 __attribute__((aligned(align)))
#define ALIGN_0x40                   ALIGN(0x40)

int haxHandle;

DECL_FUNCTION(void, PPCHalt) {
    //apparently the below line isn't needed after all? it seemed to be at one point, gonna leave it here in case lol
    //OSSendAppSwitchRequest(0,0,0); //from what I can tell, staying in the crashed process causes the reboot to fail. So I switch to something else (HOME Menu overlay)
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
    ALIGN_0x40 uint32_t arguments[0x40 >> 2];
    arguments[0] = 0x74;

    ALIGN_0x40 int result[0x40 >> 2];
    int res = IOS_Ioctl(haxHandle, IOCTL_SVC, arguments, (1 + 0) * 4, result, 4);
    return;
}

INITIALIZE_PLUGIN() {
}

DEINITIALIZE_PLUGIN() {
}

ON_APPLICATION_START() {
    WHBLogUdpInit();
    WHBLogCafeInit();
    DEBUG_FUNCTION_LINE("HaltFix loaded");

    //this ensures the reboot works by giving this plugin a direct iosuhax handle. idk why, but the one from libmocha sometimes seems to... vanish? by the time the reboot is attempted??
    haxHandle = IOS_Open((char *) ("/dev/iosuhax"), static_cast<IOSOpenMode>(0));
    DEBUG_FUNCTION_LINE("haxHandle %x!", haxHandle);

    auto res = Mocha_InitLibrary();

    if (res != MOCHA_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("Mocha init failed with code %d!", res);
        return;
    }
}

ON_APPLICATION_ENDS() {
    WHBLogUdpDeinit();
    WHBLogCafeDeinit();
    Mocha_DeInitLibrary();
}

WUPS_MUST_REPLACE_FOR_PROCESS(PPCHalt, WUPS_LOADER_LIBRARY_COREINIT, PPCHalt, WUPS_FP_TARGET_PROCESS_GAME);