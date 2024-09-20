#pragma once
#include <glfw/glfw3.h>

namespace engine
{
    enum class GamepadButton : uint16_t
    {
        A               = GLFW_GAMEPAD_BUTTON_A,
        B               = GLFW_GAMEPAD_BUTTON_B,
        X               = GLFW_GAMEPAD_BUTTON_X,
        Y               = GLFW_GAMEPAD_BUTTON_Y,
        LeftBumper      = GLFW_GAMEPAD_BUTTON_LEFT_BUMPER,
        RightBumper     = GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,
        Back            = GLFW_GAMEPAD_BUTTON_BACK,
        Start           = GLFW_GAMEPAD_BUTTON_START,
        Guide           = GLFW_GAMEPAD_BUTTON_GUIDE,
        LeftThumbstick  = GLFW_GAMEPAD_BUTTON_LEFT_THUMB,
        RightThumbstick = GLFW_GAMEPAD_BUTTON_RIGHT_THUMB,
        D_Up            = GLFW_GAMEPAD_BUTTON_DPAD_UP,
        D_Right         = GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
        D_Down          = GLFW_GAMEPAD_BUTTON_DPAD_DOWN,
        D_Left          = GLFW_GAMEPAD_BUTTON_DPAD_LEFT,

        Cross    = A,
        Circle   = B,
        Square   = X,
        Triangle = Y,
        L1       = LeftBumper,
        L3       = LeftThumbstick,
        R1       = RightBumper,
        R3       = RightThumbstick,
    };

    enum class GamepadAxis
    {
        Left_X       = GLFW_GAMEPAD_AXIS_LEFT_X,
        Left_Y       = GLFW_GAMEPAD_AXIS_LEFT_Y,
        Right_X      = GLFW_GAMEPAD_AXIS_RIGHT_X,
        Right_Y      = GLFW_GAMEPAD_AXIS_RIGHT_Y,
        LeftTrigger  = GLFW_GAMEPAD_AXIS_LEFT_TRIGGER,
        RightTrigger = GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER,

        L2 = LeftTrigger,
        R2 = RightTrigger,
    };
} // namespace engine