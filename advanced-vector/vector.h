#pragma once

#pragma once

#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <type_traits>
#include <algorithm>
#include  <iterator>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    RawMemory(const RawMemory& other) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept : buffer_(other.buffer_), capacity_(other.capacity_) {
        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if(buffer_) Deallocate(buffer_);
        buffer_ = rhs.buffer_;
        rhs.buffer_ = nullptr;
        capacity_ = rhs.capacity_;
        return *this;
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)  //
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)  //
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept {
        return iterator{ data_.GetAddress() };
    }

    iterator end() noexcept {
        return iterator{ data_ + size_ };
    }

    const_iterator cbegin() const noexcept {
        return const_iterator{ data_.GetAddress() };
    }

    const_iterator cend() const noexcept {
        return const_iterator{ data_ + size_ };
    }

    const_iterator begin() const noexcept {
        return cbegin();
    }

    const_iterator end() const noexcept {
        return cend();
    }
    
    
    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    Vector(Vector&& other) noexcept : data_(std::move(other.data_)), size_(std::move(other.size_)) {
        other.size_ = 0;
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            }
            else {
                /* Скопировать элементы из rhs, создав при необходимости новые
                   или удалив существующие */
                size_t i = 0;
                if (rhs.size_ < size_) {
                    for (; i != rhs.size_; ++i) {
                        data_[i] = rhs.data_[i];
                    }
                    std::destroy_n(data_ + i, size_ - rhs.size_);
                }
                else {
                    for (; i != size_; ++i) {
                        data_[i] = rhs.data_[i];
                    }
                    std::uninitialized_copy_n(rhs.data_ + i, rhs.size_ - size_, data_ + i);
                }
                size_ = rhs.size_;
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
        }
        return *this;
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    void Resize(size_t new_size) {
        if (new_size < size_) {
            std::destroy_n(data_ + new_size, size_ - new_size);
        }
        else if (new_size > size_) {
            if (new_size > Capacity()) {
                Reserve(new_size);
            }
            std::uninitialized_value_construct_n(data_ + size_, new_size - size_);
        }
        size_ = new_size;
    }
        
    void PopBack() /* noexcept */ {
        assert(size_ != 0);
        Destroy(data_.GetAddress() + size_ - 1);
        --size_;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ < Capacity()) {
            new (data_ + size_) T(std::forward<Args>(args)...);
        }
        else {
            RawMemory<T> new_data(size_ == 0 ? 1 : 2 * size_);
            new (new_data + size_) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
                
            }
            else {
                try {
                    std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
                }
                catch (...) {
                    Destroy(new_data + size_);
                    throw;
                }
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_ = std::move(new_data);
        }
        ++size_;
        return data_[size_ - 1];
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        assert(begin() <= pos && pos <= end());
        size_t dist = pos - begin();
        if (size_ < Capacity()) {
            if (size_ == 0) {
                new (data_ + size_) T(std::forward<Args>(args)...);
            }
            else {
                T tmp = T(std::forward<Args>(args)...);
                std::uninitialized_move_n(std::prev(end()), 1, end());
                std::move_backward(begin() + dist, std::prev(end()), end());
                data_[dist] = std::move(tmp);
            }
        }
        else {
            RawMemory<T> new_data(size_ == 0 ? 1 : 2 * size_);
            new (new_data + dist) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), dist, new_data.GetAddress());
                std::uninitialized_move_n(data_.GetAddress() + dist, size_ - dist, new_data.GetAddress() + dist + 1);
            }
            else {
                try {
                    std::uninitialized_copy_n(data_.GetAddress(), dist, new_data.GetAddress());
                    std::uninitialized_copy_n(data_.GetAddress() + dist, size_ - dist, new_data.GetAddress() + dist + 1);
                }
                catch (...) {
                    Destroy(new_data + size_);
                    throw;
                }
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_ = std::move(new_data);
        }
        ++size_;
        return &data_[dist];
    }


    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    void PushBack(const T& value) {
       EmplaceBack(value);
    }

    void PushBack(T&& value) {
        EmplaceBack(std::move(value));
    }
        
    iterator Erase(const_iterator pos) {
        assert(begin() <= pos && pos <= end());
        size_t dist = pos - begin();
        std::move(begin() + dist + 1, end(), begin() + dist);
        Destroy(data_ + size_ - 1);
        --size_;
        return &data_[dist];
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;

    // Создаёт копию объекта elem в сырой памяти по адресу buf
    static void CopyConstruct(T* buf, const T& elem) {
        new (buf) T(elem);
    }

    // Вызывает деструктор объекта по адресу buf
    static void Destroy(T* buf) noexcept {
        buf->~T();
    }


};