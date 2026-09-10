#ifndef RINGBUFFER_MAIN_H
#define RINGBUFFER_MAIN_H
#include <vector>
#include <stdexcept>

template<typename T>
class RingBuffer {
    size_t capacity_{0}, size_{0};
    std::vector<T> data;
    size_t head_{0}, tail_{0};

public:
    RingBuffer(const size_t &capacity) : capacity_(capacity) {data.resize(capacity_);}

    void push_back(const T &x) {
        const bool isFull = full();
        data[tail_] = x;
        tail_ = (tail_ + 1) % capacity_;
        if (!isFull) ++size_;
        else head_ = (head_ + 1) % capacity_;
    }

    void push_back(T&& x) {
        const bool isFull = full();
        data[tail_] = std::move(x);
        tail_ = (tail_ + 1) % capacity_;
        if (!isFull) ++size_;
        else head_ = (head_ + 1) % capacity_;
    }

    void pop_back() {
        if (empty()) return;
        tail_ = tail_ == 0 ? capacity_ - 1 : tail_ - 1;
        --size_;
    }

    void pop_front() {
        if (empty()) return;
        head_ = (head_ + 1) % capacity_;
        --size_;
    }

    void bulk_remove(const size_t &count) {
        if (empty()) return;
        if (count >= size_) {clear(); return;}
        head_ = (head_ + count) % capacity_;
        size_ -= count;
    }

    void clear() {
        head_ = tail_ = 0;
        size_ = 0;
    }

    T& operator[](size_t index) {
        if (index >= size_ && !full()) throw std::out_of_range("Tried to access a member of the buffer out of bounds");
        return data[(head_ + index) % capacity_];
    }

    const T& operator[](size_t index) const {
        if (index >= size_ && !full()) throw std::out_of_range("Tried to access a member of the buffer out of bounds");
        return data[(head_ + index) % capacity_];
    }

    T& front() {return data[head_];}

    T& back() {return tail_ == 0 ? data[capacity_ - 1] : data[tail_ - 1];}

    bool empty() const {return size_ == 0;}
    bool full() const {return size_ == capacity_;}

    size_t size() const {return size_;}
    size_t capacity() const {return capacity_;}
};



#endif //RINGBUFFER_MAIN_H