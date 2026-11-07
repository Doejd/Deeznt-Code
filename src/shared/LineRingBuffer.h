#ifndef GODOT_TERMINAL_LINERINGBUFFER_H
#define GODOT_TERMINAL_LINERINGBUFFER_H

#include <vector>
#include <godot_cpp/templates/vector.hpp>
#include "Segment.h"


class LineRingBuffer {
    size_t capacity_{0}, size_{0};
    std::vector<godot::Vector<Segment>> data;
    size_t head{0};

    [[nodiscard]] size_t GetPhysicalIndex(size_t index) const {
        return (head + index) % capacity_;
    }

public:
    explicit LineRingBuffer(const size_t &capacity) : capacity_(capacity) , data(capacity_) {}
    ~LineRingBuffer() = default;

    void clear() {
        for (auto &buf : data) buf.clear();
        head = 0;
        size_ = 0;
    }

    godot::Vector<Segment>& operator[](size_t index) {
        return data[GetPhysicalIndex(index)];
    }

    const godot::Vector<Segment>& operator[](size_t index) const {
        return data[GetPhysicalIndex(index)];
    }

    [[nodiscard]] size_t size() const {return size_;}
    [[nodiscard]] size_t capacity() const {return capacity_;}

    [[nodiscard]] bool empty() const {return size_ == 0;}
    [[nodiscard]] bool full() const {return size_ == capacity_;}

    void pushSegment(const Segment& seg) {
        if (empty()) initNewLine();
        data[GetPhysicalIndex(size_ - 1)].push_back(seg);
    }

    void clearLastLine() {
        if (empty()) initNewLine();
        data[GetPhysicalIndex(size_ - 1)].clear();
    }

    void initNewLine() {
        if (capacity_ == 0) return;

        size_t index;

        if (size_ < capacity_) {
            index = GetPhysicalIndex(size_);
            ++size_;
        }
        else {
            head = (head + 1) % capacity_;
            index = GetPhysicalIndex(size_ - 1);
        }

        data[index].clear();
    }
};


#endif //GODOT_TERMINAL_LINERINGBUFFER_H
