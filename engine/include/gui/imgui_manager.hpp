#pragma once
#include "backend/descriptor_pool.hpp"
#include "backend/vulkan_backend.hpp"
#include <imgui.h>
#include <memory>

namespace engine
{
    class ImGuiManager
    {
      public:
        ImGuiManager();
        ImGuiManager(std::shared_ptr<VulkanBackend> backend, GLFWwindow *p_window);
        ~ImGuiManager();

        void init(std::shared_ptr<VulkanBackend> backend, GLFWwindow *p_window);
        void destroy();

        void make_current();

        void new_frame();
        void render(struct DrawingContext &render_context);

      private:
        std::shared_ptr<RenderDeviceManager> m_device_manager;
        DescriptorPoolManager                m_descriptor_pool;
        ImGuiContext                        *mp_context;
    };
} // namespace engine
