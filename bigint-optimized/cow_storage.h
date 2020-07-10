//
// Created by ahmad on 10.07.2020.
//

#ifndef BIGINT_COW_STORAGE_H
#define BIGINT_COW_STORAGE_H


#include <memory>
#include <vector>

// storage with only copy-on-write optimisation

template<typename T>
struct cow_storage {
  cow_storage();
  cow_storage& operator=(const cow_storage &other);

  size_t size() const;
  void resize(size_t new_sz);
  void resize(size_t new_sz, T fill_value);
  void push_back(const T &val);
  void clear();
  const T& operator[](size_t id) const;
  T& operator[](size_t id);
  const T& back() const;

  std::shared_ptr< std::vector<T> > common_buff;

private:
  void unify_if_needed();
};

template<typename T>
cow_storage<T>::cow_storage() {
  common_buff.reset(new std::vector<T>());
}

template<typename T>
cow_storage<T> &cow_storage<T>::operator=(
        const cow_storage &other) {
  if (this == &other) {
    return *this;
  }
  common_buff = other.common_buff;
  return *this;
}

template<typename T>
size_t cow_storage<T>::size() const {
  return common_buff->size();
}

template<typename T>
void cow_storage<T>::resize(size_t new_sz) {
  if (new_sz != common_buff->size()) {
    unify_if_needed();
    common_buff->resize(new_sz);
  }
}

template<typename T>
void cow_storage<T>::resize(size_t new_sz, T fill_value) {
  if (new_sz != common_buff->size()) {
    unify_if_needed();
    common_buff->resize(new_sz, fill_value);
  }
}

template<typename T>
void cow_storage<T>::push_back(const T &val) {
  unify_if_needed();
  common_buff->push_back(val);
}

template<typename T>
void cow_storage<T>::clear() {
  unify_if_needed();
  common_buff->clear();
}

template<typename T>
const T &cow_storage<T>::operator[](size_t id) const {
  return (*common_buff)[id];
}

template<typename T>
T &cow_storage<T>::operator[](size_t id) {
  unify_if_needed();
  return (*common_buff)[id];
}

template<typename T>
const T &cow_storage<T>::back() const {
  return common_buff->back();
}

template<typename T>
void cow_storage<T>::unify_if_needed() {
  if (!common_buff.unique()) {
    auto *new_buffer = new std::vector<T>(*common_buff);
    common_buff.reset(new_buffer);
  }
}

#endif //BIGINT_COW_STORAGE_H
