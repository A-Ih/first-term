#ifndef VECTOR_H
#define VECTOR_H

#include <cstddef>
#include <stdexcept>
#include <sstream>
#include <cassert>

template <typename T>
struct vector
{
    using iterator = T*;
    using const_iterator = const T*;

    vector() = default;                     // O(1) nothrow
    vector(const vector &other);            // O(N) strong
    vector& operator=(const vector &other); // O(N) strong

    ~vector();                              // O(N) nothrow

    T& operator[](size_t i);                // O(1) nothrow
    const T& operator[](size_t i) const;    // O(1) nothrow

    T* data();                              // O(1) nothrow
    const T* data() const;                  // O(1) nothrow
    size_t size() const;                    // O(1) nothrow

    T& front();                             // O(1) nothrow
    const T& front() const;                 // O(1) nothrow

    T& back();                              // O(1) nothrow
    const T& back() const;                  // O(1) nothrow
    void push_back(const T &val);           // O(1)* strong
    void pop_back();                        // O(1) nothrow

    bool empty() const;                     // O(1) nothrow

    size_t capacity() const;                // O(1) nothrow
    void reserve(size_t);                   // O(N) strong
    void shrink_to_fit();                   // O(N) strong

    void clear();                           // O(N) nothrow

    void swap(vector &other);               // O(1) nothrow

    iterator begin();                       // O(1) nothrow
    iterator end();                         // O(1) nothrow

    const_iterator begin() const;           // O(1) nothrow
    const_iterator end() const;             // O(1) nothrow

    iterator insert(iterator pos, const T &val);       // O(N) weak
    iterator insert(const_iterator pos, const T &val); // O(N) weak

    iterator erase(iterator pos);           // O(N) weak
    iterator erase(const_iterator pos);     // O(N) weak

    iterator erase(iterator first, iterator last);             // O(N) weak
    iterator erase(const_iterator first, const_iterator last); // O(N) weak

private:
  void ensure_capacity(size_t);
  T* new_buffer(size_t new_capacity) const;
  void destroy_elems(iterator start,  size_t len) const;
  void delete_buffer(T* buff, size_t len) const;
  void replace_buffer(size_t new_cap);

private:
  T* data_ = nullptr;
  size_t size_ = 0;
  size_t capacity_ = 0;
};

template<typename T>
void vector<T>::destroy_elems(iterator start, size_t len) const {
  for (size_t i = len; i > 0; i--) {
    (start + i - 1)->~T();
  }
}

template<typename T>
void vector<T>::delete_buffer(T *buff, size_t len) const {
  destroy_elems(buff, len);
  operator delete(buff);
}

template<typename T>
T* vector<T>::new_buffer(size_t new_capacity) const {
  T* new_space = new_capacity == 0 ?
          nullptr : static_cast<T*>(operator new(sizeof(T) * new_capacity));
  size_t i = 0;
  try {
    for (; i < size_; i++) {
        new(new_space + i)T(data_[i]);
    }
  } catch(...) {
    delete_buffer(new_space, i);
    throw;
  }
  return new_space;
}

template<typename T>
void vector<T>::replace_buffer(size_t new_cap) {
  T* was = data_;
  data_ = new_buffer(new_cap);
  delete_buffer(was, size_);
  capacity_ = new_cap;
}

template<typename T>
void vector<T>::ensure_capacity(size_t new_cap) {
  if (new_cap > capacity_) {
    replace_buffer(capacity_ * 2 < new_cap ? new_cap : capacity_ * 2);
  }
}

template<typename T>
vector<T>::vector(const vector<T> &other) {
  data_ = other.new_buffer(other.size_);
  size_ = other.size_;
  capacity_ = size_;
}

template<typename T>
vector<T>& vector<T>::operator=(const vector<T> &other) {
  if (this == &other) {
    return *this;
  }
  vector<T> copy(other);
  swap(copy);
  return *this;
}

template<typename T>
vector<T>::~vector() {
  delete_buffer(data_, size_);
}

template<typename T>
T& vector<T>::operator[](size_t i) {
  assert(i < size_);
  return data_[i];
}

template<typename T>
const T& vector<T>::operator[](size_t i) const {
  assert(i < size_);
  return data_[i];
}

template<typename T>
T* vector<T>::data() {
  return data_;
}

template<typename T>
const T* vector<T>::data() const {
  return data_;
}

template<typename T>
size_t vector<T>::size() const {
  return size_;
}

template<typename T>
T& vector<T>::front() {
  assert(size_ != 0);
  return data_[0];
}

template<typename T>
const T& vector<T>::front() const {
  assert(size_ != 0);
  return data_[0];
}

template<typename T>
T& vector<T>::back() {
  assert(size_ != 0);
  return data_[size_ - 1];
}

template<typename T>
const T& vector<T>::back() const {
  assert(size_ != 0);
  return data_[size_ - 1];
}

template<typename T>
void vector<T>::push_back(const T &val) {
  insert(end(), val);
}

template<typename T>
void vector<T>::pop_back() {
  assert(size_ != 0);
  erase(end() - 1);
}

template<typename T>
bool vector<T>::empty() const {
  return size_ == 0;
}

template<typename T>
size_t vector<T>::capacity() const {
  return capacity_;
}

template<typename T>
void vector<T>::reserve(size_t demanded_capacity) {
  if (demanded_capacity > capacity_) {
    replace_buffer(demanded_capacity);
  }
}

template<typename T>
void vector<T>::shrink_to_fit() {
  if (size_ < capacity_) {
    replace_buffer(size_);
  }
}

template<typename T>
void vector<T>::clear() {
  erase(begin(), end());
}

template<typename T>
typename vector<T>::iterator vector<T>::begin() {
  return data_;
}

template<typename T>
typename vector<T>::iterator vector<T>::end() {
  return data_ + size_;
}

template<typename T>
typename vector<T>::const_iterator vector<T>::begin() const {
  return data_;
}

template<typename T>
typename vector<T>::const_iterator vector<T>::end() const {
  return data_ + size_;
}

template<typename T>
typename vector<T>::iterator vector<T>::insert(vector<T>::iterator pos, const T &val) {
  return insert(static_cast<const_iterator>(pos), val);
}

template<typename T>
typename vector<T>::iterator vector<T>::insert(vector<T>::const_iterator pos, const T &val) {
  size_t position = pos - begin();
  if (size_ == capacity_) {
    T copy = val;
    ensure_capacity(size_ + 1);
    new(data_ + size_)T(copy);
  } else {
    new(data_ + size_)T(val);
  }
  size_++;
  for (size_t i = size_ - 1; i >= position + 1; i--) {
    std::swap(data_[i], data_[i - 1]);
  }
  return data_ + position;
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(vector<T>::iterator pos) {
  return erase(pos, pos + 1);
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(vector<T>::const_iterator pos) {
  return erase(const_cast<iterator>(pos));
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(vector<T>::iterator first, vector<T>::iterator last) {
  return erase(static_cast<const_iterator>(first), static_cast<const_iterator>(last));
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(
        vector::const_iterator first,
        vector::const_iterator last) {
  ptrdiff_t diff = last - first;
  ptrdiff_t pos_first = first - begin();
  ptrdiff_t to_end = end() - first;
  for (size_t i = 0; i != to_end - diff; i++) {
    std::swap(data_[pos_first + i], data_[pos_first + i + diff]);
  }
  destroy_elems(end() - diff, diff);
  size_ -= diff;
  return begin() + pos_first + diff;
}

template<typename T>
void vector<T>::swap(vector<T> &other) {
  std::swap(size_, other.size_);
  std::swap(capacity_, other.capacity_);
  std::swap(data_, other.data_);
}

#endif // VECTOR_H
