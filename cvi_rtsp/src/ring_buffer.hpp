#pragma once

#include <atomic>
#include <mutex>
#include <memory>
#include <condition_variable>

template <typename T>
class RingBuffer {
    public:
        RingBuffer(int capacity=5);
        ~RingBuffer();
        void put(const T &data);
        bool get(T &data);
        void clear();
        void abort();
        size_t size() const;
        bool empty() const;
        bool isAbort() const;
    private:
        bool _empty() const;
    private:
        std::unique_ptr<T[]> _buf;
        unsigned int _capacity;
        unsigned int _head;
        unsigned int _tail;
        std::atomic<bool> _full;
        std::atomic<bool> _abort;
        mutable std::mutex _mutex;
        std::condition_variable _cond;
};

template <typename T>
RingBuffer<T>::RingBuffer(int capacity):
    _buf(std::unique_ptr<T[]>(new T[capacity])),
    _capacity(capacity), _head(0), _tail(0), _full(0), _abort(false)
{
}

template <typename T>
RingBuffer<T>::~RingBuffer()
{
    abort();
}

template <typename T>
void RingBuffer<T>::clear()
{
    std::lock_guard<std::mutex> lock(_mutex);

    _head = _tail;
    _full = false;
}

template <typename T>
void RingBuffer<T>::put(const T &data)
{
    std::lock_guard<std::mutex> lock(_mutex);

    _buf[_head] = data;

    _head = (_head + 1) % _capacity;
    if (_full) {
        _tail = (_tail + 1) % _capacity;
    }
    _full = _head == _tail;

    _cond.notify_one();
}

template <typename T>
bool RingBuffer<T>::get(T &data)
{
    std::unique_lock<std::mutex> lock(_mutex);

    while (!_abort && _empty()) {
        _cond.wait(lock);
    }

    if (_abort) {
        return false;
    }

    data = _buf[_tail];

    _tail = (_tail + 1) % _capacity;

    _full = false;

    return true;
}

template <typename T>
bool RingBuffer<T>::empty() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return (!_full) && (_head == _tail);
}

template <typename T>
bool RingBuffer<T>::_empty() const
{
    return (!_full) && (_head == _tail);
}

template <typename T>
size_t RingBuffer<T>::size() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    size_t size = _capacity;
    if (!_full) {
        size = _head > _tail?
            _head - _tail:
            _capacity + _head - _tail;
    }

    return size;
}

template <typename T>
void RingBuffer<T>::abort()
{
    _abort = true;
    _cond.notify_all();
}

template <typename T>
bool RingBuffer<T>::isAbort() const
{
    return _abort;
}

