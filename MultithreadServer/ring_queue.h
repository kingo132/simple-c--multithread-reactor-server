#ifndef RING_QUEUE_H
#define RING_QUEUE_H

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>

#include "socket_info.h"

// Enumeration of block types
enum class BlockType : char {
    Data,    // Data block
    Padding, // Padding block for alignment
    Final    // End of message block, indicating connection closure
};

// Structure of a data block in the ring queue
struct QueueBlock {
    uint32_t total_length;      // Total length of the block, including header and data
    uint64_t block_id;          // Block ID
    BlockType type;             // Type of the data block
    SocketInfo socket_info;     // Socket information associated with this block
    uint16_t accept_fd;         // Socket accepting the client connection
    char data[];                // Variable-length data part
};

class RingQueue {
public:
    RingQueue(size_t buffer_size);  // buffer_size represents the total size of the queue
    ~RingQueue();

    // Push a data block into the queue
    bool push(const char* data, size_t length, const QueueBlock& block_header);
    // Pop a data block from the queue, returns the actual data length
    bool wait_and_pop(char* data, size_t max_buffer_size, size_t& actual_length, QueueBlock& block_header, std::chrono::milliseconds timeout);

    // Reserved functions: for future expansion of padding functionality
    void enable_padding(bool enable);
    void insert_padding_if_needed();

private:
    size_t buffer_size_;  // Total size of the ring queue
    char* buffer_;        // Continuous memory block
    std::atomic<size_t> write_index_;
    std::atomic<size_t> read_index_;
    std::mutex mutex_;
    std::condition_variable cond_var_;

    bool padding_enabled_;  // Indicates if padding is enabled

    size_t get_free_space() const;
    size_t get_used_space() const;
};

#endif // RING_QUEUE_H
