#pragma once

#include <util/generic/deque.h>
#include <util/generic/maybe.h>
#include <util/system/yassert.h>

// TODO (a-sidorin@) : TVectorOps.
template <typename T, size_t SmallSize>
class TSmallStack {
public:
    TSmallStack() = default;
    TSmallStack(TSmallStack&&) = delete;
    TSmallStack(const TSmallStack&) = delete;
    TSmallStack& operator=(TSmallStack&&) = delete;
    TSmallStack& operator=(const TSmallStack&&) = delete;

    class const_iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;

        explicit const_iterator(const TSmallStack& parent)
            : Parent{parent}
            , Index{0}
        {
        }
        const_iterator(const const_iterator& rhs) = default;
        const_iterator(const TSmallStack& parent, size_t index)
            : Parent{parent}
            , Index{index} {
        }
        const_iterator& operator=(const const_iterator& rhs) = default;

        bool operator==(const const_iterator& rhs) const {
            Y_ASSERT(&Parent == &rhs.Parent);
            return Index == rhs.Index;
        }

        bool operator!=(const const_iterator& rhs) const {
            return !(*this == rhs);
        }

        const_iterator& operator++() {
            ++Index;
            return *this;
        }

        const const_iterator operator++(int) {
            const_iterator ret(*this);
            ++Index;
            return ret;
        }
        const_iterator& operator+=(difference_type n) {
            Index += n;
            return *this;
        }
        const_iterator operator+(difference_type n) {
            const_iterator ret(*this);
            ret += n;
            return ret;
        }
        const_iterator& operator-=(difference_type n) {
            Index -= n;
            return *this;
        }
        const_iterator operator-(difference_type n) {
            const_iterator ret(*this);
            ret -= n;
            return ret;
        }
        difference_type operator-(const const_iterator& rhs) {
            return Index - rhs.Index;
        }
        const T& operator*() const {
            return Parent[Index];
        }

    private:
        const TSmallStack& Parent;
        size_t Index;
    };

    const_iterator begin() const {
        return const_iterator{*this};
    }
    const_iterator end() const {
        return const_iterator{*this, Size};
    }

    template <typename... TArgs>
    T& emplace_back(TArgs&&... args) {
        auto newIndex = Size;
        ++Size;
        if (Y_LIKELY(IsSmall())) {
            T& result = SmallStorage[newIndex];
            result = T{std::forward<TArgs>(args)...};
            return result;
        }

        const size_t largeIndex = GetLargeIndex(newIndex);
        if (largeIndex < LargeStorage.size()) {
            T& result = LargeStorage[largeIndex] = T{std::forward<TArgs>(args)...};
            return result;
        }
        return LargeStorage.emplace_back(std::forward<TArgs>(args)...);
    }

    T& back() {
        Y_ASSERT(!empty());
        return (*this)[Size - 1];
    }

    const T& back() const {
        Y_ASSERT(!empty());
        return (*this)[Size - 1];
    }

    void pop_back() {
        Y_ASSERT(!empty());
        --Size;
        // NOTE (a-sidorin@): We reuse the memory allocated in the large deque and don't clear it.
    }

    T& operator[](size_t index) {
        Y_ASSERT(index < Size);
        if (Y_LIKELY(index < SmallSize)) {
            return SmallStorage[index];
        }
        return LargeStorage[GetLargeIndex(index)];
    }

    const T& operator[](size_t index) const {
        Y_ASSERT(index < Size);
        if (Y_LIKELY(index < SmallSize)) {
            return SmallStorage[index];
        }
        return LargeStorage[GetLargeIndex(index)];
    }

    void clear() noexcept {
        Size = 0;
        // NOTE (a-sidorin@): We reuse some memory buckets allocated in the large deque and don't clear it completely.
        if (LargeStorage.size() > SmallSize) {
            LargeStorage.resize(SmallSize);
            LargeStorage.shrink_to_fit();
        }
    }

    size_t size() const noexcept {
        return Size;
    }

    bool empty() const {
        return Size == 0;
    }

private:
    static size_t GetLargeIndex(size_t size) {
        Y_ASSERT(size >= SmallSize);
        return size - SmallSize;
    }

    constexpr bool IsSmall() const noexcept {
        return Size <= SmallSize;
    }

private:
    friend class TSmallStackTest;

    T SmallStorage[SmallSize];
    TDeque<T> LargeStorage;
    size_t Size = 0;
};
