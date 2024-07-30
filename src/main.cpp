#include "main.h"

#include <windows.h>
#include <stdio.h>
#include <unordered_map>
#include <sstream>

#include "camera.h"

ModMeta __stdcall GetModInfo() {
    static ModMeta meta = {
        "Free Camera",
        "FreeCam",
        "1.0.0",
        "Kapilarny"
    };

    return meta;
}

std::string to_hex_str(__int64 num) {
    std::stringstream stream;
    stream << std::hex << num;
    return stream.str();
}

struct mat4x4 {
    float m[4][4];
};

struct vec3f {
    float x, y, z;
};

// 772910
// __int64 __fastcall sub_772910(__int64 a1, vec3f* a2)
typedef __int64(__fastcall* ASBR_Camera_SetPos)(__int64 a1, vec3f* a2);
ASBR_Camera_SetPos ASBR_Camera_SetPos_Original;

__int64 __fastcall ASBR_Camera_SetPos_Hook(__int64 a1, vec3f* a2)
{
    JAPI_LogInfo("ASBR_Camera_SetPos(" + std::to_string(a1) + ", [" + std::to_string(a2->x) + ", " + std::to_string(a2->y) + ", " + std::to_string(a2->z) + "])");

    return ASBR_Camera_SetPos_Original(a1, a2);
}

vec3f free_cam_pos = {69, 69, 69};
vec3f free_cam_rot = {0, 0, 0};
// vec3f LockedLookAt = {220, -915, 162};

// 7727E0
// double __fastcall ASBR_Camera_Update(__int64 a1)
typedef double(__fastcall* ASBR_Camera_Update)(__int64 a1);
ASBR_Camera_Update ASBR_Camera_Update_Original;

double __fastcall ASBR_Camera_Update_Hook(__int64 a1)
{
    // Get the camera position (at a1 + 112)
    vec3f* pos = (vec3f*)(a1 + 112);
    vec3f* lookAt = (vec3f*)(a1 + 124);
    // vec3f* tilt = (vec3f*)(a1 + 148);

    // Grab the matrix
    mat4x4* matrix = (mat4x4*)(a1 + 0x30);

    if(pos->x != 640 || pos->y != 360 || pos->z != -320) {
        pos->x = free_cam_pos.x;
        pos->y = free_cam_pos.y;
        pos->z = free_cam_pos.z;

        // Set the look at forward
        lookAt->x = pos->x + free_cam_rot.x; // x
        lookAt->y = pos->y + 50; // z
        lookAt->z = pos->z + free_cam_rot.y; // y

        // Remove tilt
        matrix->m[0][0] = 1;
        matrix->m[1][0] = 0;
        matrix->m[0][2] = 0;
        matrix->m[1][2] = 1;

        JAPI_LogInfo("ASBR_Camera_Update(" + to_hex_str(a1) + ") -> [" + std::to_string(pos->x) + ", " + std::to_string(pos->y) + ", " + std::to_string(pos->z) + "]");
    }

    // JAPI_LogInfo("ASBR_Camera_Update(" + std::to_string(a1) + ")");

    return ASBR_Camera_Update_Original(a1);
}

// 772930
// __int64 __fastcall ASBR_Camera_SetLookAt(__int64 a1, __int64 a2)
typedef __int64(__fastcall* ASBR_Camera_SetLookAt)(__int64 a1, vec3f* a2);
ASBR_Camera_SetLookAt ASBR_Camera_SetLookAt_Original;

__int64 __fastcall ASBR_Camera_SetLookAt_Hook(__int64 a1, vec3f* a2)
{
    JAPI_LogInfo("ASBR_Camera_SetLookAt(" + to_hex_str(a1) + ", [" + std::to_string(a2->x) + ", " + std::to_string(a2->y) + ", " + std::to_string(a2->z) + "])");

    return ASBR_Camera_SetLookAt_Original(a1, a2);
}

// 66185C
// LRESULT __fastcall ASBR_HWNDProcCallback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)

typedef LRESULT(__fastcall* ASBR_HWNDProcCallback)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
ASBR_HWNDProcCallback ASBR_HWNDProcCallback_Original;

static Camera camera({69, 69, 69});

static int res_width = 0;
static int res_height = 0;
static bool disable_ui = false;

static char FORWARD_KEY = 'I';
static char BACKWARD_KEY = 'K';
static char LEFT_KEY = 'J';
static char RIGHT_KEY = 'L';

bool free_cam_enabled = false;

LRESULT __fastcall ASBR_HWNDProcCallback_Hook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    constexpr float speed = 20;

    // if key is pressed, toggle free cam
    if(msg == WM_KEYUP && wParam == VK_F5) {
        free_cam_enabled = !free_cam_enabled;
        JAPI_LogInfo("Free cam: " + std::to_string(free_cam_enabled));
    
        if(free_cam_enabled) {
            // Make the cursor invisible
            ShowCursor(FALSE);

            // Get the window size
            RECT rect;
            GetWindowRect(hWnd, &rect);

            // Calculate the center of the window
            int centerX = rect.left + (rect.right - rect.left) / 2;
            int centerY = rect.top + (rect.bottom - rect.top) / 2;

            // Set the mouse position to the center
            SetCursorPos(centerX, centerY);
        }
    }

    if(!free_cam_enabled) {
        ShowCursor(TRUE);
        return ASBR_HWNDProcCallback_Original(hWnd, msg, wParam, lParam);
    }

    // If I/J/K/L are pressed, move the camera
    if(msg == WM_KEYDOWN) {
        if(wParam == 'I') {
            camera.ProcessKeyboard(FORWARD, speed);
        } else if(wParam == 'K') {
            camera.ProcessKeyboard(BACKWARD, speed);
        } else if(wParam == 'J') {
            camera.ProcessKeyboard(LEFT, speed);
        } else if(wParam == 'L') {
            camera.ProcessKeyboard(RIGHT, speed);
        }
    }

    if(msg == WM_MOUSEMOVE) {
        // Get the mouse position
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        // Get the window size
        RECT rect;
        GetWindowRect(hWnd, &rect);

        // Calculate the center of the window
        int centerX = rect.left + (rect.right - rect.left) / 2;
        int centerY = rect.top + (rect.bottom - rect.top) / 2;

        // Set the mouse position to the center
        SetCursorPos(centerX, centerY);

        // Get Cursor position in screen coordinates
        POINT p;
        GetCursorPos(&p);

        // Convert screen coordinates to client coordinates
        ScreenToClient(hWnd, &p);

        // Calculate the difference
        int diffX = x - p.x;
        int diffY = y - p.y;

        // Rotate the camera
        // JAPI_LogInfo("Mouse moved: " + std::to_string(diffX) + ", " + std::to_string(diffY));
        camera.ProcessMouseMovement(-diffX, diffY);
    }

    // // If mouse moved rotate the camera
    // if(msg == WM_MOUSEMOVE) {
    //     // Get the mouse position
    //     int x = LOWORD(lParam);
    //     int y = HIWORD(lParam);

    //     // Get the window size
    //     RECT rect;
    //     GetClientRect(hWnd, &rect);

    //     // Calculate the center of the window
    //     int centerX = rect.left + (rect.right - rect.left) / 2;
    //     int centerY = rect.top + (rect.bottom - rect.top) / 2;

    //     // Calculate the difference
    //     int diffX = x - centerX;
    //     int diffY = y - centerY;

    //     // Set the mouse position to the center
    //     SetCursorPos(centerX, centerY);



    //     // Rotate the camera
    //     free_cam_rot.y += diffX * 10.0f;
    //     free_cam_rot.x += diffY * 10.0f;
    // }

    auto result = ASBR_HWNDProcCallback_Original(hWnd, msg, wParam, lParam);
    ShowCursor(FALSE); // Make the cursor invisible

    return result;
}

// Sample Matrix
// 0.578564 -0.815623 0.004762 -99.157143
// -0.027465 -0.013646 0.999530 -147.159210
// -0.815175 -0.578422 -0.030296 -712.216980
// 0.000000 0.000000 0.000000 1.000000

void copy_mat4x4(mat4x4* dest, glm::mat4* src) {
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            dest->m[i][j] = (*src)[i][j];
        }
    }
}

// 6C9300
// _DWORD *__fastcall ASBR_CopyMatrix4x4Inversed(_DWORD *to, _DWORD *from)
typedef void*(__fastcall* ASBR_CopyMatrix4x4Inversed)(void* to, void* from);
ASBR_CopyMatrix4x4Inversed ASBR_CopyMatrix4x4Inversed_Original;

void* __fastcall ASBR_CopyMatrix4x4Inversed_Hook(void* to, void* from)
{
    return ASBR_CopyMatrix4x4Inversed_Original(to, from);
}

// 7727E0
// void* __fastcall sub_7727E0(__int64 a1)
typedef mat4x4*(__fastcall* ASBR_CreateLookAtMatrix)(__int64 a1);
ASBR_CreateLookAtMatrix ASBR_CreateLookAtMatrix_Original;

mat4x4* __fastcall ASBR_CreateLookAtMatrix_Hook(__int64 a1)
{
    vec3f* pos = (vec3f*)(a1 + 112);
    mat4x4* matrix = (mat4x4*)(a1 + 0x30);

    // JAPI_LogInfo("ASBR_CreateLookAtMatrix(" + to_hex_str(a1) + ") -> [" + std::to_string(pos->x) + ", " + std::to_string(pos->y) + ", " + std::to_string(pos->z) + "]");

    if(!free_cam_enabled) return ASBR_CreateLookAtMatrix_Original(a1);

    if(res_width && res_height && pos->x == res_width / 2 && pos->y == res_height / 2 && pos->z == -320) {
        // JAPI_LogInfo("UI Camera Call: " + std::to_string(pos->x) + ", " + std::to_string(pos->y) + ", " + std::to_string(pos->z));

        return ASBR_CreateLookAtMatrix_Original(a1);
    }

    ASBR_CreateLookAtMatrix_Original(a1);

    glm::mat4 view = camera.GetViewMatrix();

    void* matrix_ptr = (void*)matrix;
    void* view_ptr = (void*)&view[0][0];

    ASBR_CopyMatrix4x4Inversed_Original(matrix_ptr, view_ptr);

    return matrix;
}

Hook hook_ASBR_Camera_SetPos = {
    (void*)0x772910,
    (void*)ASBR_Camera_SetPos_Hook,
    (void**)&ASBR_Camera_SetPos_Original,
    "ASBR_Camera_SetPos"
};

Hook hook_ASBR_Camera_Update = {
    (void*)0x7727E0,
    (void*)ASBR_Camera_Update_Hook,
    (void**)&ASBR_Camera_Update_Original,
    "ASBR_Camera_Update"
};

Hook hook_ASBR_Camera_SetLookAt = {
    (void*)0x772930,
    (void*)ASBR_Camera_SetLookAt_Hook,
    (void**)&ASBR_Camera_SetLookAt_Original,
    "ASBR_Camera_SetLookAt"
};

Hook hook_ASBR_HWNDProcCallback = {
    (void*)0x66185C,
    (void*)ASBR_HWNDProcCallback_Hook,
    (void**)&ASBR_HWNDProcCallback_Original,
    "ASBR_HWNDProcCallback"
};

Hook hook_ASBR_CreateLookAtMatrix = {
    (void*)0x7727E0,
    (void*)ASBR_CreateLookAtMatrix_Hook,
    (void**)&ASBR_CreateLookAtMatrix_Original,
    "ASBR_CreateLookAtMatrix"
};

Hook hook_ASBR_CopyMatrix4x4Inversed = {
    (void*)0x6C9300,
    (void*)ASBR_CopyMatrix4x4Inversed_Hook,
    (void**)&ASBR_CopyMatrix4x4Inversed_Original,
    "ASBR_CopyMatrix4x4Inversed"
};

std::string char_to_string(char c) {
    std::string s;
    s.push_back(c);
    return s;
}

void __stdcall ModInit() {
    JAPI_LogInfo("BUILD: " __DATE__ " " __TIME__);

    // // MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY)
    SPEED = JAPI_ConfigBindFloat("MovementSpeed", SPEED);
    SENSITIVITY = JAPI_ConfigBindFloat("MouseSensitivity", SENSITIVITY);

    res_width = JAPI_ConfigBindInt("ResolutionWidth", 0);
    res_height = JAPI_ConfigBindInt("ResolutionHeight", 0);

    FORWARD_KEY = JAPI_ConfigBindString("KeyForward", char_to_string(FORWARD_KEY)).at(0);
    BACKWARD_KEY = JAPI_ConfigBindString("KeyBackward", char_to_string(BACKWARD_KEY)).at(0);
    LEFT_KEY = JAPI_ConfigBindString("KeyLeft", char_to_string(LEFT_KEY)).at(0);
    RIGHT_KEY = JAPI_ConfigBindString("KeyRight", char_to_string(RIGHT_KEY)).at(0);

    if(res_width != 0 && res_height != 0) {
        JAPI_LogInfo("Resolution Set: " + std::to_string(res_width) + "x" + std::to_string(res_height));
    } else {
        JAPI_LogWarn("Resolution not set! UI may not work correctly!");
    }

    if(0
        // || !JAPI_HookASBRFunction(&hook_ASBR_Camera_SetPos)
        // || !JAPI_HookASBRFunction(&hook_ASBR_Camera_Update)
        // || !JAPI_HookASBRFunction(&hook_ASBR_Camera_SetLookAt)
        || !JAPI_HookASBRFunction(&hook_ASBR_HWNDProcCallback)
        || !JAPI_HookASBRFunction(&hook_ASBR_CreateLookAtMatrix)
        || !JAPI_HookASBRFunction(&hook_ASBR_CopyMatrix4x4Inversed)
    ) {
        JAPI_LogError("Failed to apply patches!");
        return;
    }

    JAPI_LogInfo("Applied patches!");
}