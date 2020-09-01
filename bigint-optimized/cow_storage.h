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

  cow_storage() noexcept = default;
  cow_storage(const cow_storage &other) noexcept;
  cow_storage& operator=(const cow_storage &other) noexcept;
  ~cow_storage() noexcept = default;

  std::vector<T> const& read_storage();
  std::vector<T>& get_storage();

private:
  void allocate_buffer_if_empty();

  typename std::enable_if<
  std::is_fundamental<T>::value && std::is_integral<T>::value,
  std::shared_ptr< std::vector<T> > >::type common_buff;
};

template<typename T>
cow_storage<T>::cow_storage(const cow_storage &other) noexcept : common_buff(other.common_buff) {}

template<typename T>
cow_storage<T>& cow_storage<T>::operator=(const cow_storage &other) noexcept {
  cow_storage temp = other;
  using std::swap;
  swap(this->common_buff, temp.common_buff);
  return *this;
}

template<typename T>
void cow_storage<T>::allocate_buffer_if_empty() {
  if (common_buff.use_count() == 0) {
    common_buff.reset(new std::vector<T>());
  }
}

template<typename T>
std::vector<T> const& cow_storage<T>::read_storage() {
  allocate_buffer_if_empty();
  return *(common_buff.get());
}

template<typename T>
std::vector<T> &cow_storage<T>::get_storage() {
  allocate_buffer_if_empty();
  if (!common_buff.unique()) {
    common_buff.reset(new std::vector<T>(*common_buff));
  }
  return *(common_buff.get());
}

#endif //BIGINT_COW_STORAGE_H
