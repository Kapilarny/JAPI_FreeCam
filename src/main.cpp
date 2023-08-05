#include "main.h"

#include <windows.h>
#include <stdio.h>

ModMeta __stdcall GetModInfo() {
    static ModMeta meta = {
        "No Character Rendering",
        "NoCharaRender",
        "1.0.0",
        "Kapilarny"
    };

    return meta;
}

typedef __int64(__fastcall* sub_267C44)(unsigned __int64 a1);
sub_267C44 sub_267C44_Original;

__int64 __fastcall sub_267C44_Hook(unsigned __int64 a1)
{
    return 0;
}

void __stdcall ModInit() {
    Hook hook = {
        (void*)0x267C44,
        (void*)sub_267C44_Hook,
        (void**)&sub_267C44_Original,
        "sub_267C44"
    };

    if(!JAPI_HookASBRFunction(&hook)) {
        JAPI_LogError("Failed to apply hooks!");
        return;
    }

    JAPI_LogInfo("Applied patches!");
}