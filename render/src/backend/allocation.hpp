#pragma once
#include "exceptions.hpp"
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace engine
{
    struct Allocation
    {
        VmaAllocator   allocator;
        VmaAllocation  allocation;
        vk::Buffer     buffer;
        vk::DeviceSize size;

        inline Allocation(VmaAllocator allocator, vk::DeviceSize size, vk::BufferUsageFlags usage)
            : allocator(allocator)
            , allocation(nullptr)
            , buffer(nullptr)
            , size(size)
        {
            vk::BufferCreateInfo bci = {
                .size  = size,
                .usage = usage,
            };

            VmaAllocationCreateInfo vma_alloc = {
                .usage = VMA_MEMORY_USAGE_AUTO,
            };

            if (VkResult result = vmaCreateBuffer(allocator, (VkBufferCreateInfo *)&bci, &vma_alloc,
                                                  (VkBuffer *)&buffer, &allocation, nullptr))
                throw VulkanException(result, "Failed to allocate buffer");
        }

        inline Allocation(Allocation &&other)
        {
            allocator  = other.allocator;
            allocation = other.allocation;
            buffer     = other.buffer;
            size       = other.size;

            other.allocator  = nullptr;
            other.allocation = nullptr;
            other.buffer     = nullptr;
            other.size       = 0;
        }

        inline ~Allocation()
        {
            if (allocator)
                vmaDestroyBuffer(allocator, buffer, allocation);
        }
    };
} // namespace engine