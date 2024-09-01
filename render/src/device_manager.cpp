#include "device_manager.hpp"
#include <algorithm>
#include <instance_manager.hpp>

using namespace engine;

VulkanDeviceManager::VulkanDeviceManager(std::shared_ptr<VulkanInstanceManager> instance_manager,
                                                 vk::PhysicalDevice                     physical_device)
    : m_logger(instance_manager->m_logger)
    , m_instance_manager(instance_manager)
    , m_dispatch(instance_manager->m_dispatch)
    , m_physical_device(physical_device)
{
    // Make sure that the device passed is available.
    if constexpr (DEBUG_ASSERTIONS) {
        auto devices = instance_manager->enumerate_physical_devices();

        auto idx = std::find(devices.begin(), devices.end(), physical_device);
        if (idx == devices.end())
            throw Exception("The device passed to the constructor is not available to the instance manager");
    }
}

VulkanDeviceManager::~VulkanDeviceManager()
{
    m_device.destroy();
}
