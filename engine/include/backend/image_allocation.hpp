#pragma once
#include "allocator.hpp"
#include "exceptions.hpp"
#include <vulkan/vulkan.hpp>

namespace engine
{
    struct ImageAllocationInfo
    {
        vk::ImageType           type           = vk::ImageType::e2D;
        vk::ImageUsageFlags     usage          = {};
        uint32_t                width          = 0;
        uint32_t                height         = 0;
        uint32_t                depth          = 1;
        vk::Format              format         = vk::Format::eR8G8B8A8Srgb;
        vk::ImageTiling         tiling         = vk::ImageTiling::eOptimal;
        vk::ImageLayout         initial_layout = vk::ImageLayout::eUndefined;
        uint32_t                mip_levels     = 1;
        uint32_t                array_layers   = 1;
        vk::SampleCountFlagBits samples        = vk::SampleCountFlagBits::e1;
        vk::SharingMode         sharing        = vk::SharingMode::eExclusive;

        vk::ImageViewCreateFlags  view_flags             = {};
        vk::ComponentMapping      view_component_mapping = {};
        vk::ImageSubresourceRange view_subresource_range = {
            .aspectMask     = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        };
    };

    struct ImageAllocation
    {
      public:
        std::shared_ptr<VulkanAllocator> allocator;
        VmaAllocation                    allocation;
        vk::Image                        image;
        vk::ImageView                    view;
        vk::Extent2D                     extent;
        vk::Format                       format;
        vk::ImageLayout                  layout;
        vk::ImageSubresourceRange        subresource_range;

        inline operator vk::Image() { return image; }
        inline operator const vk::Image() const { return image; }

        inline operator vk::ImageView() { return view; }
        inline operator const vk::ImageView() const { return view; }

        void transition_layout(vk::ImageLayout new_layout);

        ~ImageAllocation();
        ImageAllocation();
        ImageAllocation(std::shared_ptr<VulkanAllocator> allocator, const ImageAllocationInfo &info);
        ImageAllocation(ImageAllocation &&other) noexcept;
        ImageAllocation &operator=(ImageAllocation &&other) noexcept;
    };
} // namespace engine