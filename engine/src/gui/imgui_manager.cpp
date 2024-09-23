#include "gui/imgui_manager.hpp"

#include "backend/instance_manager.hpp"
#include "constants.hpp"
#include "drawables/drawing_context.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

constexpr bool ENABLE_MULTIVIEWPORTS = true;

namespace engine
{
    ImGuiManager::ImGuiManager()
        : m_device_manager(nullptr)
        , m_descriptor_pool()
        , mp_context(nullptr)
    { }

    ImGuiManager::ImGuiManager(VulkanBackend &vk_backend, GLFWwindow *p_window)
    {
        init(vk_backend, p_window);
    }

    ImGuiManager::~ImGuiManager()
    {
        destroy();
    }

    void ImGuiManager::init(VulkanBackend &backend, GLFWwindow *p_window)
    {
        m_device_manager = backend.m_device_manager;
        // Descriptor pool as used by imgui needs the `FreeDescriptorSet` flag bit set.
        m_descriptor_pool.init(backend.m_device_manager, 64, vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

        IMGUI_CHECKVERSION();
        mp_context  = ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        if constexpr (ENABLE_MULTIVIEWPORTS)
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui_ImplGlfw_InitForVulkan(p_window, true);
        ImGui_ImplVulkan_InitInfo init = {
            .Instance        = m_device_manager->instance_manager->instance,
            .PhysicalDevice  = m_device_manager->physical_device,
            .Device          = m_device_manager->device,
            .QueueFamily     = m_device_manager->graphics_queue.index,
            .Queue           = m_device_manager->graphics_queue.handle,
            .DescriptorPool  = m_descriptor_pool.get_pool(),
            .RenderPass      = backend.m_render_pass,
            .MinImageCount   = MAX_IN_FLIGHT,
            .ImageCount      = MAX_IN_FLIGHT + 1,
            .MSAASamples     = VK_SAMPLE_COUNT_1_BIT,
            .PipelineCache   = nullptr,
            .Subpass         = 0,
            .Allocator       = nullptr,
            .CheckVkResultFn = nullptr,
        };
        ImGui_ImplVulkan_Init(&init);
    }

    void ImGuiManager::destroy()
    {
        if (!mp_context)
            return;

        make_current();
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(mp_context);
        m_descriptor_pool.destroy();

        mp_context       = nullptr;
        m_device_manager = nullptr;
    }

    void ImGuiManager::make_current()
    {
        ImGui::SetCurrentContext(mp_context);
    }

    void ImGuiManager::new_frame()
    {
        make_current();
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiManager::end_frame()
    {
        make_current();
        ImGui::EndFrame();
    }

    void ImGuiManager::update_platform_windows()
    {
        if constexpr (ENABLE_MULTIVIEWPORTS) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    void ImGuiManager::render(DrawingContext &context)
    {
        make_current();
        ImGui::Render();
        ImDrawData *draw_data = ImGui::GetDrawData();

        ImGui_ImplVulkan_RenderDrawData(draw_data, context.cmd);
    }

} // namespace engine
