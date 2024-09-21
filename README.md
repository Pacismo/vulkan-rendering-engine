# Basic Vulkan Renderer

This is a basic implementation of Vulkan to render shapes to a window.

## Building

The CMake Presets file contains the configurations necessary to build the project. Ninja is required for building any configuration.

On Windows, you need to have Visual Studio installed. If you also have MinGW or LLVM installed, please use the Visual Studio Command Prompt or PowerShell.

Configure and build. On Windows and Linux, there are three configuration modes:

- `debug` -- terminal and debug utilities
- `release` -- terminal, no debug utils
- `release-noterm` -- no terminal, no debug utils
