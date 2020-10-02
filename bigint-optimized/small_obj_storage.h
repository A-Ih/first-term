//
// Created by ahmad on 10.07.2020.
//

#ifndef BIGINT_SMALL_OBJ_STORAGE_H
#define BIGINT_SMALL_OBJ_STORAGE_H

#include <cassert>
#include "cow_storage.h"

// ***Copy-on-write and small-object optimised storage***
template<typename T>
struct small_obj_storage {

  static_assert(std::is_trivial<T>::value && std::is_trivially_destructible<T>::value,
  "Only trivially-destructible trivial types are allowed");

  small_obj_storage() = default;
  small_obj_storage(const small_obj_storage &other);
  small_obj_storage& operator=(const small_obj_storage &other);
  ~small_obj_storage();

  size_t size() const;
  void resize(size_t new_sz, const T &fill_value = T());
  void push_back(const T &val);
  void clear();
  const T& operator[](size_t id) const;
  T& operator[](size_t id);
  const T& back() const;
private:
  static constexpr size_t SMALL_OBJECT_SIZE = sizeof(cow_storage<T>*) / sizeof(T) + 1;

  void unshare();

  union united_storage {
    struct {
      T buff[SMALL_OBJECT_SIZE];
      size_t len = 0;
    } static_storage;
    cow_storage<T> *dynamic_storage;
  } small_obj_buff = {};
  bool promoted = false;
};

template<typename T>
void small_obj_storage<T>::unshare() {
  assert(promoted);
  cow_storage<T> *&ds = small_obj_buff.dynamic_storage;
  if (ds->get_use_count() == 1) {
    return;
  }
  ds->dec_counter();
  ds = new cow_storage<T>(ds->read_storage());
}

template<typename T>
small_obj_storage<T>::small_obj_storage(const small_obj_storage<T> &other) {
  if (other.size() <= SMALL_OBJECT_SIZE) {
    for (size_t i = 0; i < other.size(); i++) {
      (*this)[i] = other[i];
    }
    small_obj_buff.static_storage.len = other.size();
  } else {
    promoted = true;
    small_obj_buff = other.small_obj_buff;
    small_obj_buff.dynamic_storage->inc_counter();
  }
}

template<typename T>
small_obj_storage<T>& small_obj_storage<T>::operator=(const small_obj_storage<T> &other) {
  if (&other == this) {
    return *this;
  }
  if (promoted) {
    small_obj_buff.dynamic_storage->dec_counter();
  }
  promoted =  other.promoted;
  small_obj_buff = other.small_obj_buff;
  if (promoted) {
    small_obj_buff.dynamic_storage->inc_counter();
  }
  return *this;
}

template<typename T>
size_t small_obj_storage<T>::size() const {
  return promoted ?
  small_obj_buff.dynamic_storage->read_storage().size()
  : small_obj_buff.static_storage.len;
}

template<typename T>
void small_obj_storage<T>::resize(size_t new_sz, const T &fill_value) {
  if (promoted) {
    unshare();
    small_obj_buff.dynamic_storage->get_storage().resize(new_sz, fill_value);
  } else {
    auto &ss = small_obj_buff.static_storage;
    if (new_sz <= SMALL_OBJECT_SIZE) {
      std::fill_n(ss.buff + size(), new_sz > ss.len ? new_sz - ss.len : 0, fill_value);
      ss.len = new_sz;
    } else {
      small_obj_buff.dynamic_storage = new cow_storage<T>(ss.buff, ss.buff + ss.len);
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
  if (promoted) {
    return small_obj_buff.dynamic_storage->read_storage()[id];
  } else {
    return small_obj_buff.static_storage.buff[id];
  }
}

template<typename T>
T& small_obj_storage<T>::operator[](size_t id) {
  if (promoted) {
    unshare();
    return small_obj_buff.dynamic_storage->get_storage()[id];
  } else {
    return small_obj_buff.static_storage.buff[id];
  }
}

template<typename T>
const T& small_obj_storage<T>::back() const {
  return (*this)[size() - 1];
}

template<typename T>
small_obj_storage<T>::~small_obj_storage() {
  if (promoted) {
    small_obj_buff.dynamic_storage->dec_counter();
  }
}

#endif //BIGINT_SMALL_OBJ_STORAGE_H
