#include <iostream>
#include <cstddef>
#include <initializer_list>
#include <utility>
#include <stdexcept>

// Simple dynamic array class template (similar to a tiny std::vector)
template <typename T>
class SimpelVector {
private:
    T* m_Data;         // pointer to the allocated array
    size_t m_Size;     // number of elements currently stored
    size_t m_Capacity; // allocated capacity (number of T objects that fit without realloc)

    // Resize the internal buffer to at least new_capacity.
    // If new_capacity < m_Size, we bump it up to m_Size so we don't lose elements.
    // This implementation allocates a new dynamic array, moves existing elements into it,
    // deletes the old array and updates the pointer and capacity.
    void resize_capacity(size_t new_capacity) {
        if (new_capacity < m_Size) {
            new_capacity = m_Size;
        }

        T* new_data = new T[new_capacity];
        for (size_t i = 0; i < m_Size; ++i) {
            // Move elements into the new storage (note: requires T to be move-assignable)
            new_data[i] = std::move(m_Data[i]);
        }
        delete[] m_Data;       // free old storage
        m_Data = new_data;
        m_Capacity = new_capacity;
    }

public:
    // Forward iterator (non-const)
    class Iterator {
    private:
        T* m_Ptr;
    public:
        Iterator(T* ptr) : m_Ptr(ptr) {}
        T& operator*() const { return *m_Ptr; }             // dereference
        Iterator& operator++() { ++m_Ptr; return *this; }   // pre-increment
        Iterator operator++(int) { Iterator tmp = *this; ++m_Ptr; return tmp; } // post-increment
        Iterator& operator--() { --m_Ptr; return *this; }   // pre-decrement
        Iterator operator--(int) { Iterator tmp = *this; --m_Ptr; return tmp; } // post-decrement
        bool operator==(const Iterator& other) const { return m_Ptr == other.m_Ptr; }
        bool operator!=(const Iterator& other) const { return m_Ptr != other.m_Ptr; }
    };

    // Const iterator
    class ConstIterator {
    private:
        const T* m_Ptr;
    public:
        ConstIterator(const T* ptr) : m_Ptr(ptr) {}
        const T& operator*() const { return *m_Ptr; }
        ConstIterator& operator++() { ++m_Ptr; return *this; }
        ConstIterator operator++(int) { ConstIterator tmp = *this; ++m_Ptr; return tmp; }
        ConstIterator& operator--() { --m_Ptr; return *this; }
        ConstIterator operator--(int) { ConstIterator tmp = *this; --m_Ptr; return tmp; }
        bool operator==(const ConstIterator& other) const { return m_Ptr == other.m_Ptr; }
        bool operator!=(const ConstIterator& other) const { return m_Ptr != other.m_Ptr; }
    };

    // Default constructor: start empty
    SimpelVector() : m_Data(nullptr), m_Size(0), m_Capacity(0) {}

    // Initializer-list constructor: push each element
    SimpelVector(std::initializer_list<T> init) : SimpelVector() {
        for (const auto& val : init) {
            push_back(val);
        }
    }

    // Copy constructor: deep copy of the other SimpelVector
    SimpelVector(const SimpelVector& other) : m_Data(nullptr), m_Size(other.m_Size), m_Capacity(other.m_Capacity) {
        std::cout << "Copy constructor called" << std::endl;
        if (other.m_Data) {
            m_Data = new T[m_Capacity];
            for (size_t i = 0; i < m_Size; ++i) {
                // Copy-assign each element
                m_Data[i] = other.m_Data[i];
            }
        }
    }

    // Move constructor: take ownership of other's buffer and leave it empty
    SimpelVector(SimpelVector&& other) noexcept : m_Data(other.m_Data), m_Size(other.m_Size), m_Capacity(other.m_Capacity) {
        std::cout << "Move constructor called" << std::endl;
        other.m_Data = nullptr;
        other.m_Size = 0;
        other.m_Capacity = 0;
    }

    // Copy assignment operator
    SimpelVector& operator=(const SimpelVector& other) {
        std::cout << "Copy assignment operator called" << std::endl;
        if (this != &other) {
            // allocate new buffer (or nullptr if other has no data)
            T* new_data = other.m_Data ? new T[other.m_Capacity] : nullptr;

            if (new_data) {
                for (size_t i = 0; i < other.m_Size; ++i) {
                    new_data[i] = other.m_Data[i]; // copy elements
                }
            }

            delete[] m_Data; // free existing storage

            // replace members with the new buffer
            m_Data = new_data;
            m_Size = other.m_Size;
            m_Capacity = other.m_Capacity;
        }
        return *this;
    }

    // Move assignment operator: free current buffer, steal other's buffer
    SimpelVector& operator=(SimpelVector&& other) noexcept {
        std::cout << "Move assignment operator called" << std::endl;
        if (this != &other) {
            delete[] m_Data;                // free current storage
            m_Data = other.m_Data;          // steal pointer
            m_Size = other.m_Size;
            m_Capacity = other.m_Capacity;
            other.m_Data = nullptr;         // leave other in valid empty state
            other.m_Size = 0;
            other.m_Capacity = 0;
        }
        return *this;
    }

    // Destructor: free allocated storage
    ~SimpelVector() { delete[] m_Data; }

    // Index operator (non-const): checks bounds and returns reference
    T& operator[](size_t index) {
        if (index >= m_Size) {
            throw std::out_of_range("Index out of range");
        }
        return m_Data[index];
    }

    // Index operator (const)
    const T& operator[](size_t index) const {
        if (index >= m_Size) {
            throw std::out_of_range("Index out of range");
        }
        return m_Data[index];
    }

    // push_back for lvalue references (copy)
    void push_back(const T& value) {
        if (m_Size == m_Capacity) {
            // grow: if capacity is 0 set to 1, otherwise double
            resize_capacity(m_Capacity == 0 ? 1 : m_Capacity * 2);
        }
        m_Data[m_Size++] = value; // copy-assign into next slot
    }

    // push_back for rvalue references (move)
    void push_back(T&& value) {
        if (m_Size == m_Capacity) {
            resize_capacity(m_Capacity == 0 ? 1 : m_Capacity * 2);
        }
        m_Data[m_Size++] = std::move(value); // move-assign
    }

    // pop_back: remove last element (doesn't call destructor explicitly)
    void pop_back() {
        if (m_Size == 0) {
            throw std::out_of_range("Vector is empty");
        }
        --m_Size; // just reduce size; element's destructor is not called here
    }

    // reserve: ensure capacity is at least new_capacity
    void reserve(size_t new_capacity) {
        if (new_capacity <= m_Capacity) {
            return;
        }
        resize_capacity(new_capacity);
    }

    // reverse: create a new buffer with elements in reverse order
    // keeps the current capacity (allocates m_Capacity size)
    void reverse() {
        if (m_Size <= 1) {
            return;
        }

        T* reversed_data = new T[m_Capacity];
        for (size_t i = 0; i < m_Size; ++i) {
            // move elements from the end into the new buffer in forward order
            reversed_data[i] = std::move(m_Data[m_Size - 1 - i]);
        }

        delete[] m_Data;
        m_Data = reversed_data;
    }

    // shrink_to_fit: reduce capacity to match size (reallocates)
    void shrink_to_fit() {
        if (m_Size < m_Capacity) {
            resize_capacity(m_Size);
        }
    }

    // clear: logically remove all elements by setting size to 0
    // NOTE: this does not explicitly call element destructors
    void clear() { m_Size = 0; }

    // accessors
    size_t size() const { return m_Size; }
    size_t capacity() const { return m_Capacity; }
    bool empty() const { return m_Size == 0; }

    // iterator access
    Iterator begin() { return Iterator(m_Data); }
    Iterator end() { return Iterator(m_Data + m_Size); }

    ConstIterator begin() const { return ConstIterator(m_Data); }
    ConstIterator end() const { return ConstIterator(m_Data + m_Size); }

    ConstIterator cbegin() const { return ConstIterator(m_Data); }
    ConstIterator cend() const { return ConstIterator(m_Data + m_Size); }
};

int main() {
    // create a temporary SimpelVector from an initializer_list and move it into 'vec'
    SimpelVector<size_t> vec = std::move(SimpelVector<size_t>({ 1, 2, 3, 4, 5 }));
    SimpelVector<size_t> vec2;
    vec2 = vec;    // copy assignment (deep copy)
    vec2.reverse();

    std::cout << "Vector contents: ";
    // iterate backwards by starting from end() and decrementing until begin()
    for (auto it = vec2.end(); it != vec2.begin();) {
        --it;
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    vec.push_back(6); // append element
    std::cout << "After push_back(6): ";
    for (const auto& val : vec) { // range-based for uses Iterator
        std::cout << val << " ";
    }
    std::cout << std::endl;

    vec.pop_back(); // remove last element
    std::cout << "After pop_back(): ";
    for (const auto& val : vec) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    return 0;
}