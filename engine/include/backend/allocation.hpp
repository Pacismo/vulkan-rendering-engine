#pragma once
#include "allocator.hpp"
#include "exceptions.hpp"
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace engine
{
    struct Allocation
    {
      public:
        std::shared_ptr<VulkanAllocator> allocator;
        VmaAllocation                    allocation;
        vk::Buffer                       buffer;
        vk::DeviceSize                   size;

        inline operator vk::Buffer() { return buffer; }
        inline operator const vk::Buffer() const { return buffer; }

        inline Allocation()
            : allocator(nullptr)
            , allocation(nullptr)
            , buffer(nullptr)
            , size(0)
        { }

        inline Allocation(std::shared_ptr<VulkanAllocator> allocator, vk::DeviceSize size, vk::BufferUsageFlags usage)
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

            if (VkResult result = vmaCreateBuffer(*allocator, (VkBufferCreateInfo *)&bci, &vma_alloc,
                                                  (VkBuffer *)&buffer, &allocation, nullptr))
                throw VulkanException(result, "Failed to allocate buffer");
        }

        inline Allocation(Allocation &&other) noexcept
        {
            allocator  = other.allocator;
            allocation = other.allocation;
            buffer     = other.buffer;
            size       = other.size;

            other.allocation = nullptr;
            other.buffer     = nullptr;
            other.size       = 0;
        }

        inline Allocation &operator=(Allocation &&other) noexcept
        {
            allocator = other.allocator;
            std::swap(allocation, other.allocation);
            std::swap(buffer, other.buffer);
            std::swap(size, other.size);

            return *this;
        }

        inline ~Allocation()
        {
            if (allocator)
                vmaDestroyBuffer(*allocator, buffer, allocation);
        }

      protected:
        inline Allocation(std::shared_ptr<VulkanAllocator> allocator, vk::DeviceSize size)
            : allocator(allocator)
            , allocation(nullptr)
            , buffer(nullptr)
            , size(size)
        { }
    };

    struct HostVisibleAllocation : public Allocation
    {
      public:
        bool  coherent;
        bool  random_access;
        void *p_mapping;

        inline void       *get_map() { return p_mapping; }
        inline const void *get_map() const { return p_mapping; }

        inline operator void *() { return get_map(); }
        inline operator const void *() const { return get_map(); }

        inline void flush() { flush(0, size); }
        inline void flush(vk::DeviceSize offset, vk::DeviceSize size)
        {
            if (!coherent)
                vmaFlushAllocation(*allocator, allocation, offset, size);
        }

        inline HostVisibleAllocation()
            : Allocation()
            , coherent(false)
            , random_access(false)
            , p_mapping(nullptr)
        { }

        inline HostVisibleAllocation(std::shared_ptr<VulkanAllocator> allocator, vk::DeviceSize size,
                                     vk::BufferUsageFlags usage,
                                     bool random_access = false)
            : Allocation(allocator, size)
            , random_access(random_access)
        {
            vk::BufferCreateInfo bci = {
                .size  = size,
                .usage = usage,
            };

            VmaAllocationCreateFlags access = random_access ? VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
                                                            : VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

            VmaAllocationCreateInfo vma_alloc = {
                .flags          = access | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                .usage          = VMA_MEMORY_USAGE_AUTO,
                .preferredFlags = VkMemoryPropertyFlags(vk::MemoryPropertyFlagBits::eHostCoherent),
            };

            VmaAllocationInfo alloc_info;

            if (vmaCreateBuffer(*allocator, (VkBufferCreateInfo *)&bci, &vma_alloc, (VkBuffer *)&buffer, &allocation,
                                &alloc_info)) {
                vma_alloc.preferredFlags = 0;
                if (VkResult result = vmaCreateBuffer(*allocator, (VkBufferCreateInfo *)&buffer, &vma_alloc,
                                                      (VkBuffer *)&buffer, &allocation, &alloc_info))
                    throw VulkanException(result, "Failed to create staging buffer");

                coherent = false;
            } else
                coherent = true;

            p_mapping = alloc_info.pMappedData;
        }

        inline HostVisibleAllocation(HostVisibleAllocation &&other) noexcept
            : Allocation(std::move(other))
        {
            coherent  = other.coherent;
            p_mapping = other.p_mapping;

            other.coherent  = false;
            other.p_mapping = nullptr;
        }

        inline HostVisibleAllocation &operator=(HostVisibleAllocation &&other) noexcept
        {
            allocator  = other.allocator;
            allocation = other.allocation;
            buffer     = other.buffer;
            size       = other.size;
            coherent   = other.coherent;
            p_mapping  = other.p_mapping;

            other.allocator  = nullptr;
            other.allocation = nullptr;
            other.buffer     = nullptr;
            other.size       = 0;
            other.coherent   = false;
            other.p_mapping  = nullptr;

            return *this;
        }
    };

    template<class T>
    struct TypedHostVisibleAllocation : public HostVisibleAllocation
    {
      public:
        inline TypedHostVisibleAllocation()
            : HostVisibleAllocation()
        { }

        inline TypedHostVisibleAllocation(std::shared_ptr<VulkanAllocator> allocator, vk::BufferUsageFlags usage,
                                          bool random_access = false)
            : HostVisibleAllocation(allocator, sizeof(T), usage, random_access)
        { }

        inline operator T *() { return (T *)p_mapping; }
        inline operator const T *() const { return (const T *)p_mapping; }

        inline T       *operator->() { return *this; }
        inline const T *operator->() const { return *this; }
    };

    template<class T>
    struct TypedHostVisibleAllocation<T[]> : public HostVisibleAllocation
    {
      public:
        inline TypedHostVisibleAllocation()
            : HostVisibleAllocation()
        { }

        inline TypedHostVisibleAllocation(std::shared_ptr<VulkanAllocator> allocator, vk::DeviceSize count,
                                          vk::BufferUsageFlags usage,
                                          bool random_access = false)
            : HostVisibleAllocation(allocator, sizeof(T) * count, usage, random_access)
        { }

        inline T       *get() { return (T *)get_map(); }
        inline const T *get() const { return (const T *)get_map(); }

        inline operator T *() { return get(); }
        inline operator const T *() const { return get(); }

        inline size_t           count() const { return size / type_size(); }
        inline constexpr size_t offset(size_t index) const { return index * sizeof(T); }
        inline constexpr size_t type_size() const { return sizeof(T); }

        inline T &operator[](size_t i)
        {
            assert(i < count());
            return get()[i];
        }

        inline const T &operator[](size_t i) const
        {
            assert(i < count());
            return get()[i];
        }

        inline T *begin() { return get(); }
        inline T *end() { return get() + count(); }

        inline const T *begin() const { return get(); }
        inline const T *end() const { return get() + count(); }
    };

    template<class T, size_t COUNT>
    struct TypedHostVisibleAllocation<T[COUNT]> : public TypedHostVisibleAllocation<T[]>
    {
      public:
        inline TypedHostVisibleAllocation()
            : TypedHostVisibleAllocation<T[]>()
        { }

        inline TypedHostVisibleAllocation(std::shared_ptr<VulkanAllocator> allocator, vk::BufferUsageFlags usage,
                                          bool random_access = false)
            : TypedHostVisibleAllocation<T[]>(allocator, COUNT, usage, random_access)
        { }
    };
} // namespace engine