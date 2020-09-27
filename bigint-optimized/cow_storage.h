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

  cow_storage() noexcept : common_buff(), counter(1) {}
  explicit cow_storage(const std::vector<T> &other) : common_buff(other), counter(1) {}
  cow_storage(T *begin, T *end) : common_buff(begin, end), counter(1) {}
  cow_storage(const cow_storage &other) = delete;
  cow_storage& operator=(const cow_storage &other) = delete;
  ~cow_storage() noexcept = default;

  std::vector<T> const& read_storage() const {
    assert(counter > 0);
    return common_buff;
  }

  std::vector<T>& get_storage() {
    assert(counter > 0);
    return common_buff;
  }

  void inc_counter() noexcept {
    assert(counter > 0);
    counter++;
  }

  void dec_counter() noexcept {
    assert(counter > 0);
    counter--;
    if (counter == 0) {
      delete this;
    }
  }

  size_t get_use_count() noexcept {
    assert(counter > 0);
    return counter;
  }

private:

  std::vector<T> common_buff;
  size_t counter;
};

#endif //BIGINT_COW_STORAGE_H
