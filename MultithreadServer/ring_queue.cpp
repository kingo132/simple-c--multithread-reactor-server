#include "ring_queue.h"
#include <cstring>  // for memcpy
#include <cstdlib>  // for malloc and free

RingQueue::RingQueue(size_t buffer_size)
    : buffer_size_(buffer_size), padding_enabled_(false) {
    buffer_ = (char*)malloc(buffer_size_);
    write_index_ = 0;
    read_index_ = 0;
}

RingQueue::~RingQueue() {
    free(buffer_);
}

// Get the remaining space in the ring queue
size_t RingQueue::get_free_space() const {
    size_t current_write = write_index_.load(std::memory_order_acquire);
    size_t current_read = read_index_.load(std::memory_order_acquire);
    if (current_write >= current_read) {
        return buffer_size_ - (current_write - current_read);
    } else {
        return current_read - current_write;
    }
}

// Get the used space in the queue
size_t RingQueue::get_used_space() const {
    size_t current_write = write_index_.load(std::memory_order_acquire);
    size_t current_read = read_index_.load(std::memory_order_acquire);
    if (current_write >= current_read) {
        return current_write - current_read;
    } else {
        return buffer_size_ - (current_read - current_write);
    }
}

// Push a data block into the queue
bool RingQueue::push(const char* data, size_t length, const QueueBlock& block_header) {
    std::unique_lock<std::mutex> lock(mutex_);

    size_t total_length = length + sizeof(QueueBlock);

    if (total_length > buffer_size_) {
        return false;  // Block size exceeds the entire buffer
    }

    size_t current_write = write_index_.load(std::memory_order_acquire);
    size_t free_space = get_free_space();

    // Check if there is enough free space
    if (free_space < total_length) {
        return false;  // Not enough free space
    }

    // Calculate the current write position
    size_t write_pos = current_write % buffer_size_;

    // Calculate the remaining space at the end of the buffer
    size_t remaining_space_at_end = buffer_size_ - write_pos;

    if (remaining_space_at_end >= total_length) {
        // First, write the header information
        std::memcpy(buffer_ + write_pos, &block_header, sizeof(QueueBlock));
        // Then write the data
        if (block_header.total_length > sizeof(QueueBlock)) {
            std::memcpy(buffer_ + write_pos + sizeof(QueueBlock), data, length);
        }
    } else {
        // The data crosses the end of the buffer, write it in two parts
        std::memcpy(buffer_ + write_pos, &block_header, remaining_space_at_end);
        std::memcpy(buffer_, (char*)&block_header + remaining_space_at_end, sizeof(QueueBlock) - remaining_space_at_end);
        if (block_header.total_length > sizeof(QueueBlock)) {
            std::memcpy(buffer_, data, length);
        }
    }

    // Update the write index
    write_index_.store(current_write + total_length, std::memory_order_release);

    // Notify waiting consumers that data is available
    cond_var_.notify_one();

    return true;
}

// Pop a data block from the queue
bool RingQueue::wait_and_pop(char* data, size_t max_buffer_size, size_t& actual_length, QueueBlock& block_header, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);

    while (true) {
        size_t current_read = read_index_.load(std::memory_order_acquire);
        size_t current_write = write_index_.load(std::memory_order_acquire);

        // If there is data to read
        if (current_read != current_write) {
            size_t used_space = get_used_space();

            // Read the header information to get the block length
            if (used_space > sizeof(QueueBlock)) {
                // First, read the header information
                size_t read_pos = current_read % buffer_size_;
                std::memcpy(&block_header, buffer_ + read_pos, sizeof(QueueBlock));

                actual_length = block_header.total_length - sizeof(QueueBlock);

                if (actual_length > max_buffer_size) {
                    return false;  // The provided buffer is not large enough
                }

                // Calculate remaining space
                size_t remaining_space_at_end = buffer_size_ - read_pos;

                if (remaining_space_at_end >= block_header.total_length) {
                    // Read the data
                    std::memcpy(data, buffer_ + read_pos + sizeof(QueueBlock), actual_length);
                } else {
                    // Read in two parts
                    std::memcpy(data, buffer_ + read_pos + sizeof(QueueBlock), remaining_space_at_end - sizeof(QueueBlock));
                    std::memcpy(data + remaining_space_at_end - sizeof(QueueBlock), buffer_, actual_length - (remaining_space_at_end - sizeof(QueueBlock)));
                }

                // Update the read index
                read_index_.store(current_read + block_header.total_length, std::memory_order_release);
                return true;
            } else {
                // Not enough space, might be padding or invalid data
                return false;
            }
        } else {
            // If there is no data, wait
            if (cond_var_.wait_for(lock, timeout) == std::cv_status::timeout) {
                return false;  // Timeout
            }
        }
    }
}

// Reserved function: enable or disable padding
void RingQueue::enable_padding(bool enable) {
    padding_enabled_ = enable;
}

// Reserved function: insert padding block if needed
void RingQueue::insert_padding_if_needed() {
    if (padding_enabled_) {
        // Logic to insert padding block, currently empty
    }
}