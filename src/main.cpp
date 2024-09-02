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
        "1.1.0",
        "Kapilarny & Kojo Bailey"
    };

    return meta;
}

std::string char_to_string(char c) {
    std::string s;
    s.push_back(c);
    return s;
}

struct mat4x4 {
    float m[4][4];
};

struct vec3f {
    float x, y, z;
};

struct {
    bool left = false;
    bool right = false;
    bool forward = false;
    bool backward = false;
    bool up = false;
    bool down = false;
} KeyPressed;

vec3f free_cam_pos = {69, 69, 69};
vec3f free_cam_rot = {0, 0, 0};

typedef LRESULT(__fastcall* ASBR_HWNDProcCallback)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
ASBR_HWNDProcCallback ASBR_HWNDProcCallback_Original;

static Camera camera({69, 69, 69});

static int res_width = 0;
static int res_height = 0;
static bool disable_ui = false;

static char* LEFT_KEY = new char[255];
static char* RIGHT_KEY = new char[255];
static char* FORWARD_KEY = new char[255];
static char* BACKWARD_KEY = new char[255];
static char* UP_KEY = new char[255];
static char* DOWN_KEY = new char[255];
static char* BLOCK_CAMERA_KEY = new char[255];

bool free_cam_enabled = false;
bool movement_blocked = false;

LRESULT __fastcall ASBR_HWNDProcCallback_Hook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // if key is pressed, toggle free cam
    if (msg == WM_KEYUP && wParam == VK_F5) {
        free_cam_enabled = !free_cam_enabled;
        JAPI_LogInfo("Free cam: " + std::to_string(free_cam_enabled));
    
        if (free_cam_enabled) {
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

    if (!free_cam_enabled) {
        ShowCursor(TRUE);
        return ASBR_HWNDProcCallback_Original(hWnd, msg, wParam, lParam);
    }

    if(msg == WM_KEYDOWN && wParam == BLOCK_CAMERA_KEY[0]) {
        movement_blocked = !movement_blocked;
    }

    if(movement_blocked) {
        ShowCursor(TRUE);
        return ASBR_HWNDProcCallback_Original(hWnd, msg, wParam, lParam);
    }

    // Record camera movement inputs.
    if (msg == WM_KEYDOWN) {
        if (wParam == RIGHT_KEY[0])    KeyPressed.right = true;
        if (wParam == LEFT_KEY[0])     KeyPressed.left = true;
        if (wParam == FORWARD_KEY[0])  KeyPressed.forward = true;
        if (wParam == BACKWARD_KEY[0]) KeyPressed.backward = true;
        if (wParam == UP_KEY[0])    KeyPressed.right = true;
        if (wParam == DOWN_KEY[0])     KeyPressed.left = true;
    }

    if (msg == WM_KEYUP) {
        if (wParam == RIGHT_KEY[0])    KeyPressed.right = false;
        if (wParam == LEFT_KEY[0])     KeyPressed.left = false;
        if (wParam == FORWARD_KEY[0])  KeyPressed.forward = false;
        if (wParam == BACKWARD_KEY[0]) KeyPressed.backward = false;
        if (wParam == UP_KEY[0])    KeyPressed.right = false;
        if (wParam == DOWN_KEY[0])     KeyPressed.left = false;
    }

    if (msg == WM_MOUSEMOVE) {
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
        camera.ProcessMouseMovement(-diffX, diffY);
    }

    auto result = ASBR_HWNDProcCallback_Original(hWnd, msg, wParam, lParam);
    ShowCursor(FALSE); // Make the cursor invisible

    return result;
}

void copy_mat4x4(mat4x4* dest, glm::mat4* src) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            dest->m[i][j] = (*src)[i][j];
        }
    }
}

typedef void*(__fastcall* ASBR_CopyMatrix4x4Inversed)(void* to, void* from);
ASBR_CopyMatrix4x4Inversed ASBR_CopyMatrix4x4Inversed_Original;

void* __fastcall ASBR_CopyMatrix4x4Inversed_Hook(void* to, void* from) {
    return ASBR_CopyMatrix4x4Inversed_Original(to, from);
}

typedef mat4x4*(__fastcall* ASBR_CreateLookAtMatrix)(__int64 a1);
ASBR_CreateLookAtMatrix ASBR_CreateLookAtMatrix_Original;

mat4x4* __fastcall ASBR_CreateLookAtMatrix_Hook(__int64 a1) {
    vec3f* pos = (vec3f*)(a1 + 112);
    mat4x4* matrix = (mat4x4*)(a1 + 0x30);

    // If free cam isn't currently active...
    if (!free_cam_enabled)
        return ASBR_CreateLookAtMatrix_Original(a1);

    // If is HUD camera, determined based on its consistent position...
    if (res_width && res_height && pos->x == res_width / 2 && pos->y == res_height / 2 && pos->z == -320) {
        return ASBR_CreateLookAtMatrix_Original(a1);
    }

    // Move the camera with corresponding inputs.
    // camera.ChangeX(KeyPressed.right - KeyPressed.left);
    camera.ChangeZ(KeyPressed.forward - KeyPressed.backward);
    camera.ChangeY(KeyPressed.up - KeyPressed.down);

    ASBR_CreateLookAtMatrix_Original(a1);

    glm::mat4 view = camera.GetViewMatrix();

    void* matrix_ptr = (void*)matrix;
    void* view_ptr = (void*)&view[0][0];

    ASBR_CopyMatrix4x4Inversed_Original(matrix_ptr, view_ptr);

    return matrix;
}

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

void __stdcall ModInit() {
    JAPI_LogInfo("BUILD: " __DATE__ " " __TIME__);

    // Load configurations
        // Camera properties
        JAPI_ConfigRegisterFloat(&SPEED, "MovementSpeed", SPEED);
        JAPI_ConfigRegisterFloat(&SENSITIVITY, "MouseSensitivity", SENSITIVITY);

        // Target resolution
        JAPI_ConfigRegisterInt(&res_width, "ResolutionWidth", res_width);
        JAPI_ConfigRegisterInt(&res_height, "ResolutionHeight", res_height);

        // Camera input keys
        JAPI_ConfigRegisterString(LEFT_KEY, "KeyLeft", "J");
        JAPI_ConfigRegisterString(RIGHT_KEY, "KeyRight", "L");
        JAPI_ConfigRegisterString(FORWARD_KEY, "KeyForward", "I");
        JAPI_ConfigRegisterString(BACKWARD_KEY, "KeyBackward", "K");
        JAPI_ConfigRegisterString(UP_KEY, "KeyUp", "U");
        JAPI_ConfigRegisterString(DOWN_KEY, "KeyDown", "O");
        JAPI_ConfigRegisterString(BLOCK_CAMERA_KEY, "BlockCameraKey", "B");

    if (res_width && res_height) {
        JAPI_LogInfo("Resolution specified: " + std::to_string(res_width) + "x" + std::to_string(res_height));
    } else {
        JAPI_LogWarn("Resolution not specified! HUD may not work correctly.");
    }

    if (0
        || !JAPI_HookASBRFunction(&hook_ASBR_HWNDProcCallback)
        || !JAPI_HookASBRFunction(&hook_ASBR_CreateLookAtMatrix)
        || !JAPI_HookASBRFunction(&hook_ASBR_CopyMatrix4x4Inversed)
    ) {
        JAPI_LogError("Failed to apply patches!");
        return;
    }
    JAPI_LogInfo("Applied patches!");
}