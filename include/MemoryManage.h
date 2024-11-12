#pragma once
#include <iostream>
#include <vector>

class MemoryManager
{
public:
    MemoryManager() : currentIndex(0) {}

    void allocateMemory(size_t size)
    {
        memoryPool.resize(size); // 分配指定大小的内存
        currentIndex = 0;        // 重置当前索引
    }

    void *getMemoryBlock(size_t size)
    {
        if (!isMemoryAllocated())
        {
            throw std::runtime_error("Memory not allocated."); // 如果未分配内存，抛出异常
        }

        if (currentIndex + size > memoryPool.size())
        { // 检查是否需要扩展内存
            resizeMemoryPool((currentIndex + size) * 2);
        }

        size_t startAddress = currentIndex;
        addresses.push_back(startAddress);                          // 记录相对地址
        currentIndex += size;                                       // 更新当前索引
        return reinterpret_cast<void *>(&memoryPool[startAddress]); // 返回内存池中的地址
    }

    const std::vector<size_t> getAddresses() const
    {
        return addresses;
        // std::vector<void *> absoluteAddresses;
        // for (size_t relativeAddress : addresses)
        // {
        //     absoluteAddresses.push_back((void *)&memoryPool[relativeAddress]);
        // }
        // return absoluteAddresses; // 返回所有绝对地址
    }

    void *getAddressOffset(size_t offset)
    {
        if (!isMemoryAllocated())
        {
            throw std::runtime_error("Memory not allocated."); // 如果未分配内存，抛出异常
        }

        if (offset >= memoryPool.size())
        {
            throw std::out_of_range("Offset exceeds allocated memory."); // 检查偏移量是否超出范围
        }

        return reinterpret_cast<void *>(&memoryPool[0]) + offset; // 返回首地址加上偏移量的地址
    }
    size_t getOffsetFromBase(void *address)
    {
        if (!isMemoryAllocated())
        {
            throw std::runtime_error("Memory not allocated."); // 如果未分配内存，抛出异常
        }

        if (address < reinterpret_cast<void *>(&memoryPool[0]) ||
            address >= reinterpret_cast<void *>(&memoryPool[0]) + memoryPool.size())
        {
            throw std::out_of_range("Address is outside the allocated memory range."); // 检查地址是否在范围内
        }

        return static_cast<size_t>(static_cast<char *>(address) - &memoryPool[0]); // 计算并返回偏移量
    }
    bool isMemoryAllocated() const
    {
        return !memoryPool.empty(); // 检查内存池是否为空
    }
    size_t getRemainingSpace() const
    {
        if (!isMemoryAllocated())
        {
            throw std::runtime_error("Memory not allocated."); // 如果未分配内存，抛出异常
        }

        return memoryPool.size() - currentIndex; // 返回剩余空间大小
    }
    void ensureCapacity(size_t need_space)
    {
        if (!isMemoryAllocated())
        {
            throw std::runtime_error("Memory not allocated."); // 如果未分配内存，抛出异常
        }

        if (getRemainingSpace() < need_space) // 检查剩余空间是否足够
        {
            size_t newSize = currentIndex + need_space; // 计算新的大小为当前索引加上需要的空间

            if (newSize < 1)
                newSize = 1; // 确保至少为1字节

            resizeMemoryPool(newSize); // 扩展内存池到新的大小
        }
    }
    void clear()
    {
        memoryPool.resize(0); // 清除内存池并释放内存
        addresses.clear();    // 清除记录的地址
        memoryPool.shrink_to_fit();
        currentIndex = 0; // 重置当前索引
    }



private:
    std::vector<char> memoryPool;  // 内存池
    size_t currentIndex;           // 当前索引
    std::vector<size_t> addresses; // 存储每次分配的相对地址

    void resizeMemoryPool(size_t needed)
    {
        size_t newSize = memoryPool.size() == 0 ? 1 : memoryPool.size() * 2;
        if (needed > newSize)
            newSize = needed;

        memoryPool.resize(newSize); // 扩展内存池
    }
};
