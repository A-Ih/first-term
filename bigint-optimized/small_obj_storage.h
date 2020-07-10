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

  union united_storage {
    cow_storage<T> *big_storage;
    struct {
      T buff[SMALL_OBJECT_SIZE] = {0};
      size_t len = 0;
    } small_storage{};

    T& get_ith(bool is_promoted, size_t i);
    const T& get_ith(bool is_promoted, size_t i) const;
    void promote();
  } small_obj_buff;

  bool promoted = false;
};

template<typename T>
small_obj_storage<T>::small_obj_storage(const small_obj_storage<T> &other) {
  if (this == &other) {
    return;
  }
  *this = other;
}

template<typename T>
small_obj_storage<T>& small_obj_storage<T>::operator=(const small_obj_storage<T> &other) {
  if (this == &other) {
    return *this;
  }
  if (other.size() <= SMALL_OBJECT_SIZE && !promoted) {
    for (size_t i = 0; i < other.size(); i++) {
      small_obj_buff.small_storage.buff[i] = other[i];
    }
    small_obj_buff.small_storage.len = other.size();
  } else {
    if (!promoted) {
      small_obj_buff.promote();
      promoted = true;
    }
    small_obj_buff.big_storage->resize(other.size());
    for (size_t i = 0; i < other.size(); i++) {
      (*small_obj_buff.big_storage)[i] = other[i];
    }
  }
  return *this;
}

template<typename T>
size_t small_obj_storage<T>::size() const {
  return promoted ? small_obj_buff.big_storage->size() : small_obj_buff.small_storage.len;
}

template<typename T>
void small_obj_storage<T>::resize(size_t new_sz) {
  return resize(new_sz, 0);
}

template<typename T>
void small_obj_storage<T>::resize(size_t new_sz, T fill_value) {
  if (promoted) {
    small_obj_buff.big_storage->resize(new_sz, fill_value);
  } else {
    if (new_sz <= SMALL_OBJECT_SIZE) {
      auto how_much = new_sz > size() ? new_sz - size() : 0;
      std::fill_n(small_obj_buff.small_storage.buff + size(), how_much, fill_value);
      small_obj_buff.small_storage.len = new_sz;
    } else {
      small_obj_buff.promote();
      promoted = true;
      small_obj_buff.big_storage->resize(new_sz, fill_value);
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
  return small_obj_buff.get_ith(promoted, id);
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
    delete small_obj_buff.big_storage;
  }
}

template<typename T>
void small_obj_storage<T>::united_storage::promote() {
  auto *new_buffer = new std::vector<T>(small_storage.len);
  std::copy(small_storage.buff,
            small_storage.buff + small_storage.len,
            new_buffer->begin());
  big_storage = new cow_storage<T>();
  big_storage->common_buff.reset(new_buffer);
}

template<typename T>
T& small_obj_storage<T>::united_storage::get_ith(bool is_promoted, size_t i) {
  return is_promoted ? (*big_storage)[i] : small_storage.buff[i];
}

template<typename T>
const T& small_obj_storage<T>::united_storage::get_ith(bool is_promoted, size_t i) const {
  return is_promoted ? (*big_storage)[i] : small_storage.buff[i];
}

#endif //BIGINT_SMALL_OBJ_STORAGE_H
