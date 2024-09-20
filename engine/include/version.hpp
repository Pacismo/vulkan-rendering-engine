#pragma once

#include <cstdint>

namespace engine
{
    struct Version
    {
        uint16_t major   = 0;
        uint16_t minor   = 0;
        uint16_t patch   = 0;
        uint16_t variant = 0;
    };
} // namespace engine
