#pragma once
#include <mutex>
#include <list>
#include <unordered_map>
#include "cviruntime_context.h"

struct Node {
    CVI_RT_MEM mem;
    uint64_t size;
    std::string name;
    CVI_ALLOC_TYPE type;
    bool used;
};

#define MEM_ALLOC_SIZE 120 * 1024 * 1024
#define MEM_MIN_SIZE  1024

class MemAllocate {
public:
    MemAllocate() : size_(MEM_ALLOC_SIZE), 
    min_size_(MEM_MIN_SIZE),
    mem_buf_(nullptr),
    ctx_(nullptr),
    init_(false) {} 

    ~MemAllocate() {
        for (auto& node : mem_list_) {
            CVI_RT_MemFree(ctx_, node.mem);
        }
        mem_list_.clear();
        if (mem_buf_) {
            CVI_RT_MemFree(ctx_, mem_buf_);
            mem_buf_ = nullptr;
        }
        if (ctx_) {
            CVI_RT_DeInit(ctx_);
            ctx_ = nullptr;
        }
    }

    int init() {
        if (init_) {
            return 0;
        }
        if (0 != CVI_RT_Init(&ctx_)) {
            printf("CVI_RT_Init failed\n");
            return -1;
        }
        mem_buf_ = CVI_RT_MemAlloc(ctx_, size_);
        if (nullptr == mem_buf_) {
            printf("CVI_RT_MemAlloc failed\n");
            return -1;
        }
        uint64_t remain_size = size_;
        int offset = 0;
        while (remain_size > min_size_) {
            uint64_t cur_size = remain_size >> 1;
            CVI_RT_MEM node_mem = CVI_RT_MemPreAlloc(mem_buf_, offset, cur_size);
            Node node;
            node.mem = node_mem;
            node.size = CVI_RT_MemGetSize(node_mem);
            node.used = false;
            mem_list_.emplace_front(node);
            offset += cur_size;
            remain_size -= cur_size;
            std::cout << "alloc mem block:" << cur_size << std::endl;
        }
        init_ = true;
        return 0;
    }
    CVI_RT_MEM alloc(CVI_RT_HANDLE ctx, uint64_t size, CVI_ALLOC_TYPE type, const char *name) {
        std::unique_lock<std::mutex> lk(mutex_);
        init();
        for (auto iter = mem_list_.begin(); iter != mem_list_.end(); ++iter) {
            auto& node = *iter;
            if (!node.used && node.size >= size) {
                node.used = true;
                node.type = type;
                node.name = name;
                std::cout << "alloc buf size:" << size << " node size:" << node.size << std::endl;
                return node.mem;
            }
        }
        return nullptr;
    }
    void free(CVI_RT_HANDLE ctx, CVI_RT_MEM mem) {
        std::unique_lock<std::mutex> lk(mutex_);
        for (auto &node : mem_list_) {
            if (node.mem == mem) {
                node.used = false;
                node.type = CVI_ALLOC_UNKNOWN;
                node.name = "";
                std::cout << "free buf size:" << node.size << std::endl;
                return ;
            }
        }
        return CVI_RT_MemFree(ctx, mem);
    }

    void print_stat() {
        std::cout << "*************print mem***************" << std::endl;
        std::cout << "name\t\t\t\t\t type\t\t\t\t\t size\t\t\t" << std::endl;
        std::unique_lock<std::mutex> lk(mutex_);
        for (auto &node : mem_list_) {
            if (node.used) {
                std::cout << node.name << "\t\t\t\t" << node.type << "\t\t\t\t" << node.size << std::endl;
            }
        }
        std::cout << "*************end print mem***************" << std::endl;
    }
private:
    bool init_;
    uint64_t size_;
    uint64_t min_size_;
    std::list<Node> mem_list_;
    CVI_RT_MEM mem_buf_;
    CVI_RT_HANDLE ctx_;
    std::mutex mutex_;
};

static MemAllocate gMemAllocate;
CVI_RT_MEM mem_alloc(CVI_RT_HANDLE ctx, uint64_t size, CVI_ALLOC_TYPE type, const char *name) {
    return gMemAllocate.alloc(ctx, size, type, name);
}

void mem_free(CVI_RT_HANDLE ctx, CVI_RT_MEM mem) {
    return gMemAllocate.free(ctx, mem);
}

void print_mem() {
    return gMemAllocate.print_stat();
}