//
// Created by ahmad on 10.07.2020.
//

#ifndef BIGINT_SMALL_OBJ_STORAGE_H
#define BIGINT_SMALL_OBJ_STORAGE_H

#include "cow_storage.h"

// ***Copy-on-write and small-object optimised storage***
template<typename T>
struct small_obj_storage {
  static const size_t SMALL_OBJECT_SIZE = sizeof(cow_storage<T>*) / sizeof(T) + 1;
  small_obj_storage() = default;
  small_obj_storage(const small_obj_storage &other);

  small_obj_storage& operator=(const small_obj_storage &other);

  ~small_obj_storage();

  size_t size() const;
  void resize(size_t new_sz);
  void resize(size_t new_sz, T fill_value);
  void push_back(const T &val);
  void clear();
  const T& operator[](size_t id) const;
  T& operator[](size_t id);
  const T& back() const;

private:
  union united_storage {
    struct {
      T buff[SMALL_OBJECT_SIZE];
      size_t len = 0;
    } static_storage;
    cow_storage<T> *dynamic_storage;

    T& get_ith(bool is_promoted, size_t i);
    const T& read_ith(bool is_promoted, size_t i) const;
    void promote();
  } small_obj_buff = {};

  bool promoted = false;
};

template<typename T>
small_obj_storage<T>::small_obj_storage(const small_obj_storage<T> &other) {
  if (other.size() <= SMALL_OBJECT_SIZE) {
    resize(other.size());
    for (size_t i = 0; i < other.size(); i++) {
      (*this)[i] = other[i];
    }
  } else {
    small_obj_buff.promote();
    promoted = true;
    *small_obj_buff.dynamic_storage = *other.small_obj_buff.dynamic_storage;
  }
}

template<typename T>
small_obj_storage<T>& small_obj_storage<T>::operator=(const small_obj_storage<T> &other) {
  small_obj_storage<T> temp = other;
  using std::swap;
  swap(this->promoted, temp.promoted);
  swap(this->small_obj_buff, temp.small_obj_buff);
  return *this;
}

template<typename T>
size_t small_obj_storage<T>::size() const {
  return promoted ?
  small_obj_buff.dynamic_storage->read_storage().size()
  : small_obj_buff.static_storage.len;
}

template<typename T>
void small_obj_storage<T>::resize(size_t new_sz) {
  return resize(new_sz, T());
}

template<typename T>
void small_obj_storage<T>::resize(size_t new_sz, T fill_value) {
  if (promoted) {
    small_obj_buff.dynamic_storage->get_storage().resize(new_sz, fill_value);
  } else {
    if (new_sz <= SMALL_OBJECT_SIZE) {
      auto how_much = new_sz > size() ? new_sz - size() : 0;
      std::fill_n(small_obj_buff.static_storage.buff + size(), how_much, fill_value);
      small_obj_buff.static_storage.len = new_sz;
    } else {
      small_obj_buff.promote();
      promoted = true;
      small_obj_buff.dynamic_storage->get_storage().resize(new_sz, fill_value);
    }
  }
}

template<typename T>
void small_obj_storage<T>::push_back(const T &val) {
  resize(size() + 1, val);
}

template<typename T>
void small_obj_storage<T>::clear() {
  resize(0);
}

template<typename T>
const T& small_obj_storage<T>::operator[](size_t id) const {
  return small_obj_buff.read_ith(promoted, id);
}

template<typename T>
T& small_obj_storage<T>::operator[](size_t id) {
  return small_obj_buff.get_ith(promoted, id);
}

template<typename T>
const T& small_obj_storage<T>::back() const {
  return (*this)[size() - 1];
}

template<typename T>
small_obj_storage<T>::~small_obj_storage() {
  if (promoted) {
    delete small_obj_buff.dynamic_storage;
  }
}

template<typename T>
void small_obj_storage<T>::united_storage::promote() {
  auto new_buffer = std::vector<T>(static_storage.len);
  std::copy(static_storage.buff,
            static_storage.buff + static_storage.len,
            new_buffer.begin());
  dynamic_storage = new cow_storage<T>();
  dynamic_storage->get_storage() = new_buffer;
}

template<typename T>
T& small_obj_storage<T>::united_storage::get_ith(bool is_promoted, size_t i) {
  return is_promoted ? dynamic_storage->get_storage()[i] : static_storage.buff[i];
}

template<typename T>
const T& small_obj_storage<T>::united_storage::read_ith(bool is_promoted, size_t i) const {
  return is_promoted ? dynamic_storage->read_storage()[i] : static_storage.buff[i];
}

#endif //BIGINT_SMALL_OBJ_STORAGE_H
