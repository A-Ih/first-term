#ifndef BIG_INTEGER_H
#define BIG_INTEGER_H

#include <string>
#include <cstdint>
#include <functional>
#include "small_obj_storage.h"

struct big_integer
{
  typedef uint32_t limb_t;
  typedef uint64_t dlimb_t;

  big_integer();
  big_integer(big_integer const &other) = default;
  big_integer(int a);
  explicit big_integer(std::string const &str);
  ~big_integer();

  big_integer& operator=(big_integer const &other) = default;

  big_integer& operator+=(big_integer const &rhs);
  big_integer& operator-=(big_integer const &rhs);
  big_integer& operator*=(big_integer const &rhs);
  big_integer& operator/=(big_integer const &rhs);
  big_integer& operator%=(big_integer const &rhs);

  big_integer& operator&=(big_integer const &rhs);
  big_integer& operator|=(big_integer const &rhs);
  big_integer& operator^=(big_integer const &rhs);

  big_integer& operator<<=(int rhs);
  big_integer& operator>>=(int rhs);

  big_integer operator+() const;
  big_integer operator-() const;
  big_integer operator~() const;

  big_integer& operator++();
  big_integer operator++(int);

  big_integer& operator--();
  big_integer operator--(int);

  friend bool operator==(big_integer const &a, big_integer const &b);
  friend bool operator!=(big_integer const &a, big_integer const &b);
  friend bool operator<(big_integer const &a, big_integer const &b);
  friend bool operator>(big_integer const &a, big_integer const &b);
  friend bool operator<=(big_integer const &a, big_integer const &b);
  friend bool operator>=(big_integer const &a, big_integer const &b);
  friend std::string to_string(big_integer const& a);

private:
  big_integer(limb_t a);
  size_t len() const ;
  limb_t rest_bits() const;
  bool is_negative() const;
  void new_buffer(size_t new_size);
  void make_positive();
  big_integer& make_abs();

  // division and multiplication
  void normalize();
  void mul_short(limb_t short_factor);
  void prefix(size_t len, big_integer &ans);
  void add_on_pref(big_integer const &rhs, size_t at);
  friend limb_t get_approx(big_integer const &a, big_integer const &b);
  limb_t div_short(limb_t divisor);

  // comparison
  int compare_lexicographically(big_integer const &rhs, limb_t fill_value = 0, size_t at = 0) const;
  int compare_numerically(big_integer const &rhs) const;

  // bitwise operations
  big_integer& bit_operation(
          big_integer const &rhs,
          const std::function<limb_t(limb_t, limb_t)> &f);
  big_integer& negate();

private:
  small_obj_storage<limb_t> data_;
};

big_integer operator+(big_integer a, const big_integer &b);
big_integer operator-(big_integer a, const big_integer &b);
big_integer operator*(big_integer a, const big_integer &b);
big_integer operator/(big_integer a, const big_integer &b);
big_integer operator%(big_integer a, const big_integer &b);

big_integer operator&(big_integer a, const big_integer &b);
big_integer operator|(big_integer a, const big_integer &b);
big_integer operator^(big_integer a, const big_integer &b);

big_integer operator<<(big_integer a, int b);
big_integer operator>>(big_integer a, int b);

std::ostream& operator<<(std::ostream &s, const big_integer &a);

#endif // BIG_INTEGER_H
