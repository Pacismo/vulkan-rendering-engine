#include "backend/image_allocation.hpp"

namespace engine
{
    void ImageAllocation::transition_layout(vk::ImageLayout new_layout)
    {
        auto cmd = allocator->get_device_manager()->single_time_command();

        vk::ImageMemoryBarrier imb = {
            .oldLayout           = layout,
            .newLayout           = new_layout,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image               = image,
            .subresourceRange    = subresource_range,
        };

        vk::PipelineStageFlags src = {};
        vk::PipelineStageFlags dst = {};

        if (layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal) {
            imb.srcAccessMask = vk::AccessFlagBits::eNone;
            imb.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            src = vk::PipelineStageFlagBits::eTopOfPipe;
            dst = vk::PipelineStageFlagBits::eTransfer;
        } else if (layout == vk::ImageLayout::eTransferDstOptimal
                   && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            imb.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            imb.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            src = vk::PipelineStageFlagBits::eTransfer;
            dst = vk::PipelineStageFlagBits::eFragmentShader;
        }

        cmd->pipelineBarrier(src, dst, {}, {}, {}, imb);

        cmd.submit();
    }

    ImageAllocation::ImageAllocation()
        : allocator(nullptr)
        , allocation(nullptr)
        , image(nullptr)
        , extent({0, 0})
        , format(vk::Format::eUndefined)
        , layout(vk::ImageLayout::eUndefined)
        , subresource_range({})
    { }

    ImageAllocation::ImageAllocation(std::shared_ptr<VulkanAllocator> allocator, const ImageAllocationInfo &info)
        : allocator(allocator)
        , allocation(nullptr)
        , image(nullptr)
        , view(nullptr)
        , extent({.width = info.width, .height = info.height})
        , format(info.format)
        , layout(info.initial_layout)
        , subresource_range(info.view_subresource_range)
    {
        vk::ImageCreateInfo create_info = {
            .imageType     = info.type,
            .format        = info.format,
            .extent        = {.width = info.width, .height = info.height, .depth = info.depth},
            .mipLevels     = info.mip_levels,
            .arrayLayers   = info.array_layers,
            .samples       = info.samples,
            .tiling        = info.tiling,
            .usage         = info.usage,
            .sharingMode   = info.sharing,
            .initialLayout = info.initial_layout,
        };

        VmaAllocationCreateInfo vma_alloc = {
            .flags = VkMemoryPropertyFlags(vk::MemoryPropertyFlagBits::eDeviceLocal),
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        };

        if (VkResult result = vmaCreateImage(*allocator, (VkImageCreateInfo *)&create_info, &vma_alloc,
                                             (VkImage *)&image, &allocation, nullptr))
            throw VulkanException(result, "Failed to create image");

        vk::ImageViewType ivt;
        switch (info.type) {
        case vk::ImageType::e1D:
            ivt = vk::ImageViewType::e1D;
            break;
        case vk::ImageType::e2D:
            ivt = vk::ImageViewType::e2D;
            break;
        case vk::ImageType::e3D:
            ivt = vk::ImageViewType::e3D;
            break;
        }

        vk::ImageViewCreateInfo view_info = {
            .flags            = info.view_flags,
            .image            = image,
            .viewType         = ivt,
            .format           = info.format,
            .components       = info.view_component_mapping,
            .subresourceRange = info.view_subresource_range,
        };

        view = allocator->get_device_manager()->device.createImageView(view_info);
    }

    ImageAllocation::~ImageAllocation()
    {
        if (view)
            allocator->get_device_manager()->device.destroyImageView(view);
        if (allocation)
            vmaDestroyImage(*allocator, image, allocation);

        allocator  = nullptr;
        allocation = nullptr;
        image      = nullptr;
        view       = nullptr;
    }

    ImageAllocation::ImageAllocation(ImageAllocation &&other) noexcept
    {
        allocator  = other.allocator;
        allocation = other.allocation;
        image      = other.image;
        view       = other.view;
        extent     = other.extent;

        other.allocation = nullptr;
        other.image      = nullptr;
        other.view       = nullptr;
        other.extent     = {};
    }

    ImageAllocation &ImageAllocation::operator=(ImageAllocation &&other) noexcept
    {
        allocator = other.allocator;
        std::swap(allocation, other.allocation);
        std::swap(image, other.image);
        std::swap(view, other.view);
        std::swap(extent, other.extent);

        return *this;
    }
} // namespace engine