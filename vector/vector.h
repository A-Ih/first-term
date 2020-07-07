#ifndef VECTOR_H
#define VECTOR_H

#include <cstddef>
#include <cassert>
#include <stdexcept>
#include <sstream>

template <typename T>
struct vector
{
    typedef T* iterator;
    typedef const T* const_iterator;

    vector();                               // O(1) nothrow
    vector(vector const &other);                  // O(N) strong
    vector& operator=(vector const& other); // O(N) strong

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
    void push_back(const T &val);               // O(1)* strong
    void pop_back();                        // O(1) nothrow

    bool empty() const;                     // O(1) nothrow

    size_t capacity() const;                // O(1) nothrow
    void reserve(size_t);                   // O(N) strong
    void shrink_to_fit();                   // O(N) strong

    void clear();                           // O(N) nothrow

    void swap(vector &other);                     // O(1) nothrow

    iterator begin();                       // O(1) nothrow
    iterator end();                         // O(1) nothrow

    const_iterator begin() const;           // O(1) nothrow
    const_iterator end() const;             // O(1) nothrow

    iterator insert(iterator pos, const T &val); // O(N) weak
    iterator insert(const_iterator pos, const T &val); // O(N) weak

    iterator erase(iterator pos);           // O(N) weak
    iterator erase(const_iterator pos);     // O(N) weak

    iterator erase(iterator first, iterator last); // O(N) weak
    iterator erase(const_iterator first, const_iterator last); // O(N) weak

private:
  void ensure_capacity(size_t);
  void new_buffer(size_t new_capacity);
  void destroy_elems(iterator start,  size_t len);

  // exceptions
  template<typename U>
  static std::string ptr(U *it);
  template<typename U>
  static std::string ptr(const U *it);
  void error(const std::string &message, int err_code = 0) const;
  void error(bool cond, const std::string &message, int err_code = 0) const;
  static const size_t INVALID_INDEX = 1;
  static const size_t INVALID_IT = 2;
  static const size_t INVALID_IT_RANGE = 3;

private:
  T* data_ = nullptr;
  size_t size_ = 0;
  size_t capacity_ = 0;
};

template<typename T>
template<typename U>
std::string vector<T>::ptr(U *it) {
  std::ostringstream conv;
  conv << it;
  return conv.str();
}

template<typename T>
template<typename U>
std::string vector<T>::ptr(const U *it) {
  return ptr(const_cast<U *>(it));
}

template<typename T>
void vector<T>::error(const std::string &message, int err_code) const {
  static const std::string ERRS[]  =
          {"", "invalid index: ", "invalid iterator: ", "invalid iterator range: "};
  throw std::runtime_error(ERRS[err_code]
                           + message
                           + " in vector " + ptr(this)
                           + " [size = " + std::to_string(size_) + "]");
}

template<typename T>
void vector<T>::error(bool cond, const std::string &message, int err_code) const {
  if (cond) {
    error(message, err_code);
  }
}

template<typename T>
void vector<T>::destroy_elems(iterator start, size_t len) {
  for (iterator i = start + len; i-- != start; ) {
    i->~T();
  }
}

template<typename T>
void vector<T>::new_buffer(size_t new_capacity) {
  T* new_space = new_capacity == 0 ?
          nullptr : static_cast<T*>(operator new(sizeof(T) * new_capacity));
  size_t i = 0;
  try {
    for (; i < size_; i++) {
        new(new_space + i)T(data_[i]);
    }
  } catch(...) {
    destroy_elems(new_space, i);
    operator delete(new_space);
    throw;
  }
  T* was = data_;
  data_ = new_space;
  capacity_ = new_capacity;
  destroy_elems(was, size_);
  if (was != nullptr) {
    operator delete(was);
  }
}

template<typename T>
void vector<T>::ensure_capacity(size_t new_cap) {
  if (new_cap <= capacity_) {
    return;
  }
  new_cap = capacity_ * 2 < new_cap ? new_cap : capacity_ * 2;
  new_buffer(new_cap);
}

template<typename T>
vector<T>::vector() = default;

template<typename T>
vector<T>::vector(const vector<T> &other) {
  new_buffer(other.size_);
  size_ = other.size_;
  for (size_t i = 0; i < size_; i++) {
    new(data_ + i)T(other.data_[i]);
  }
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
  clear();
  operator delete(static_cast<void*>(data_));
}

template<typename T>
T& vector<T>::operator[](size_t i) {
  error(i >= size_, std::to_string(i), INVALID_INDEX);
  return data_[i];
}

template<typename T>
const T& vector<T>::operator[](size_t i) const {
  error(i >= size_, std::to_string(i), INVALID_INDEX);
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
  error(size_ == 0, "no elements");
  return data_[0];
}

template<typename T>
const T& vector<T>::front() const {
  error(size_ == 0, "no elements");
  return data_[0];
}

template<typename T>
T& vector<T>::back() {
  error(size_ == 0, "no elements");
  return data_[size_ - 1];
}

template<typename T>
const T& vector<T>::back() const {
  error(size_ == 0, "no elements");
  return data_[size_ - 1];
}

template<typename T>
void vector<T>::push_back(const T &val) {
  insert(end(), val);
}

template<typename T>
void vector<T>::pop_back() {
  error(size_ == 0, "no elements");
  size_--;
  data_[size_].~T();
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
    new_buffer(demanded_capacity);
  }
}

template<typename T>
void vector<T>::shrink_to_fit() {
  if (size_ < capacity_) {
    new_buffer(size_);
  }
}

template<typename T>
void vector<T>::clear() {
  destroy_elems(data_, size_);
  size_ = 0;
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
  if (pos < begin() || pos > end()) {
    error(ptr(pos), INVALID_IT);
  }
  size_t position = pos - begin();
  T tempval = val;
  ensure_capacity(size_ + 1);
  new(data_ + size_)T(tempval);
  size_++;
  for (size_t i = size_; i --> position + 1;) {
    std::swap(data_[i], data_[i - 1]);
  }
  return data_ + position;
}

template<typename T>
typename vector<T>::iterator vector<T>::insert(vector<T>::const_iterator pos, const T &val) {
  return insert(const_cast<iterator>(pos), val);
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
  if (begin() > first || first > last || last > end()) {
    error(ptr(first) + ":" + ptr(last), INVALID_IT_RANGE);
  }
  if (first == last) {
    return first;
  }
  ptrdiff_t diff = last - first;
  for (iterator i = first; i != end() - diff; i++) {
    *i = *(i + diff);
  }
  destroy_elems(end() - diff, diff);
  size_ -= diff;
  return first + diff;
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(
        vector::const_iterator first,
        vector::const_iterator last) {
  return erase(const_cast<iterator>(first), const_cast<iterator>(last));
}

template<typename T>
void vector<T>::swap(vector<T> &other) {
  std::swap(size_, other.size_);
  std::swap(capacity_, other.capacity_);
  std::swap(data_, other.data_);
}

#endif // VECTOR_H