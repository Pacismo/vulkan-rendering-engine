#pragma once
#include <glfw/glfw3.h>

namespace engine
{
    enum class MouseButton
    {
        Button_1 = GLFW_MOUSE_BUTTON_1,
        Button_2 = GLFW_MOUSE_BUTTON_2,
        Button_3 = GLFW_MOUSE_BUTTON_3,
        Button_4 = GLFW_MOUSE_BUTTON_4,
        Button_5 = GLFW_MOUSE_BUTTON_5,
        Button_6 = GLFW_MOUSE_BUTTON_6,
        Button_7 = GLFW_MOUSE_BUTTON_7,
        Button_8 = GLFW_MOUSE_BUTTON_8,
        Left     = Button_1,
        Right    = Button_2,
        Middle   = Button_3,
    };
} // namespace engine