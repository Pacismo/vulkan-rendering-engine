#include "vk_result_to_string.hpp"

std::string_view result_to_string(vk::Result result) {
    switch (result) {
    case vk::Result::eSuccess:
        return "Success";
    case vk::Result::eNotReady:
        return "Not Ready";
    case vk::Result::eTimeout:
        return "Timeout";
    case vk::Result::eEventSet:
        return "Event Set";
    case vk::Result::eEventReset:
        return "Event Reset";
    case vk::Result::eIncomplete:
        return "Incomplete";
    case vk::Result::eErrorOutOfHostMemory:
        return "Host Out Of Memory";
    case vk::Result::eErrorOutOfDeviceMemory:
        return "Device Out Of Memory";
    case vk::Result::eErrorInitializationFailed:
        return "Initialization Failed";
    case vk::Result::eErrorDeviceLost:
        return "Device Lost";
    case vk::Result::eErrorMemoryMapFailed:
        return "Memory Map Failed";
    case vk::Result::eErrorLayerNotPresent:
        return "Layer Not Present";
    case vk::Result::eErrorExtensionNotPresent:
        return "Extension Not Present";
    case vk::Result::eErrorFeatureNotPresent:
        return "Feature Not Present";
    case vk::Result::eErrorIncompatibleDriver:
        return "Incompatible Driver";
    case vk::Result::eErrorTooManyObjects:
        return "Too Many Objects";
    case vk::Result::eErrorFormatNotSupported:
        return "Unsupported Format";
    case vk::Result::eErrorFragmentedPool:
        return "Fragmented Pool";
    case vk::Result::eErrorUnknown:
        return "Unknown";
    case vk::Result::eErrorOutOfPoolMemory:
        return "Out Of Pool Memory";
    case vk::Result::eErrorInvalidExternalHandle:
        return "Invalid External Handle";
    case vk::Result::eErrorFragmentation:
        return "Fragmentation";
    case vk::Result::eErrorInvalidOpaqueCaptureAddress:
        return "Invalid Opaque Capture Address";
    case vk::Result::ePipelineCompileRequired:
        return "Pipeline Compilation Required";
    case vk::Result::eErrorSurfaceLostKHR:
        return "Surface Lost";
    case vk::Result::eErrorNativeWindowInUseKHR:
        return "Native Window In Use";
    case vk::Result::eSuboptimalKHR:
        return "Suboptimal";
    case vk::Result::eErrorOutOfDateKHR:
        return "Out Of Date";
    case vk::Result::eErrorIncompatibleDisplayKHR:
        return "Incompatible Display";
    case vk::Result::eErrorValidationFailedEXT:
        return "Validation Failed";
    case vk::Result::eErrorInvalidShaderNV:
        return "Invalid Shader (NV)";
    case vk::Result::eErrorImageUsageNotSupportedKHR:
        return "Image Usage Not Supported";
    case vk::Result::eErrorVideoPictureLayoutNotSupportedKHR:
        return "Video Picture Layout Not Supported";
    case vk::Result::eErrorVideoProfileOperationNotSupportedKHR:
        return "Video Profile Operation Not Supported";
    case vk::Result::eErrorVideoProfileFormatNotSupportedKHR:
        return "Video Profile Format Not Supported";
    case vk::Result::eErrorVideoProfileCodecNotSupportedKHR:
        return "Video Profile Codec Not Supported";
    case vk::Result::eErrorVideoStdVersionNotSupportedKHR:
        return "Video Standard Version Not Supported";
    case vk::Result::eErrorInvalidDrmFormatModifierPlaneLayoutEXT:
        return "Invalid DRM Format Modifier Plane Layout";
    case vk::Result::eErrorNotPermittedKHR:
        return "Not Permitted";
#ifdef VK_USE_PLATFORM_WIN32_KHR
    case vk::Result::eErrorFullScreenExclusiveModeLostEXT:
        return "Exclusive Full Screen Mode Lost";
#endif
    case vk::Result::eThreadIdleKHR:
        return "Thread Idle";
    case vk::Result::eThreadDoneKHR:
        return "Thread Done";
    case vk::Result::eOperationDeferredKHR:
        return "Operation Deferred";
    case vk::Result::eOperationNotDeferredKHR:
        return "Operation Not Deferred";
    case vk::Result::eErrorInvalidVideoStdParametersKHR:
        return "Invalid Video Standard Parameters";
    case vk::Result::eErrorCompressionExhaustedEXT:
        return "Compression Exhausted";
    case vk::Result::eIncompatibleShaderBinaryEXT:
        return "Incompatible Shader Binary";
    }
}

std::string_view result_to_string(uint32_t result_code) {
    return result_to_string(vk::Result(result_code));
}