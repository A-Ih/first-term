#include "big_integer.h"

#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <limits>

namespace {
  using limb_t = big_integer::limb_t;
  using dlimb_t = big_integer::dlimb_t;
  const size_t LIMB_T_BITS = std::numeric_limits<limb_t>::digits;
  const limb_t LIMB_T_MAX = std::numeric_limits<limb_t>::max();
  const big_integer MOD(1000000000);  // for string converting purposes
  const size_t DECIMAL_DIGIT_LEN = 9;
  limb_t ms_bit(limb_t a) {
    return (a >> (LIMB_T_BITS - 1));
  }

  // ***helper functions***

  limb_t pow2(limb_t power) {
    return static_cast<limb_t>(1) << power;
  }

  limb_t carry(limb_t a, limb_t b, limb_t c = 0) {
    if (c <= 1) {
      return LIMB_T_MAX - a >= b && LIMB_T_MAX - a - b >= c ? 0 : 1;
    }
    return carry(a, b) + carry(a + b, c);
  }

  limb_t hi(limb_t x) {
    return (x >> (LIMB_T_BITS / 2));
  }

  limb_t lo(limb_t x) {
    return (x << (LIMB_T_BITS / 2)) >> (LIMB_T_BITS / 2);
  }

  std::pair<limb_t, limb_t> mul_limb_t(limb_t x, limb_t y) {
    limb_t a = lo(x), b = hi(x), c = lo(y), d = hi(y);
    limb_t t = a * d + b * c, tc = carry(a * d, b * c);
    limb_t lo1 = a * c, lo2 = (lo(t) << LIMB_T_BITS / 2);
    // carry(hi(t), b * d, carry(lo1, lo2)) == 0
    return {hi(t) + b * d + carry(lo1, lo2) + (tc << LIMB_T_BITS / 2), lo1 + lo2};  // {hi, lo}
  }

  void error(bool cond, std::string const &message) {
    if (cond) {
      throw std::runtime_error(message);
    }
  }
}

bool big_integer::is_negative() const {
  return ms_bit(data_[len() - 1]) == 1;
}

bool big_integer::is_zero() const {
  for (size_t i = 0; i < len(); i++) {
    if (data_[i] != 0) {
      return false;
    }
  }
  return true;
}

limb_t big_integer::rest_bits() const {
  return is_negative() ? LIMB_T_MAX : 0;
}

size_t big_integer::len() const {
  return data_.size();
}

void big_integer::new_buffer(size_t new_size) {
  if (len() < new_size) {
    data_.resize(new_size, rest_bits());
  } else {
    data_.resize(new_size);
  }
}

void big_integer::normalize() {
  if (is_zero()) {
    new_buffer(1);
    return;
  }
  size_t i = len() - 1;
  while (i > 0) {
    if (data_[i] != 0) {
      break;
    }
    i--;
  }
  new_buffer(i + 1);
}

big_integer::big_integer()
{
  data_.push_back(0);
}

big_integer::big_integer(big_integer const& other)
{
  data_ = other.data_;
}

big_integer::big_integer(limb_t a) {
  data_.push_back(a);
  if (is_negative()) {
    data_.push_back(0);
  }
}

big_integer::big_integer(int a)
{
  data_.push_back(static_cast<limb_t>(a));
}

big_integer::big_integer(std::string const& str)
{
  bool sign = (str[0] == '-');
  error(str.find_first_not_of("0123456789", sign ? 1 : 0) != std::string::npos
          || str.size() <= (sign ? 1 : 0),
        "invalid string for big_integer constructor: " + str);
  data_.push_back(0);
  for (size_t i = sign ? 1 : 0; i < str.size(); i += DECIMAL_DIGIT_LEN) {
    *this *= MOD;
    if (str.size() - i < DECIMAL_DIGIT_LEN) {
      *this /= std::stoi("1" + std::string(DECIMAL_DIGIT_LEN - (str.size() - i), '0'));
    }
    *this += std::stoi(str.substr(i, DECIMAL_DIGIT_LEN));
  }
  if (sign) {
    negate();
  }
}

big_integer::~big_integer()
{
  data_.clear();
}

big_integer& big_integer::operator=(big_integer const& other)
{
  data_ = other.data_;
  return *this;
}

// ***subtraction and addition***

void big_integer::add_on_pref(big_integer const &rhs, size_t at) {
  bool same_sign = rhs.is_negative() == is_negative();
  size_t new_length = std::max(len(), rhs.len() + at);
  new_buffer(new_length);
  auto carry_bit = 0;
  for (size_t i = at; i < len(); i++) {
    limb_t rhs_digit = i - at < rhs.len() ? rhs.data_[i - at] : rhs.rest_bits();
    auto new_carry = carry(data_[i], rhs_digit, carry_bit);
    data_[i] += rhs_digit + carry_bit;
    carry_bit = new_carry;
  }
  // "overflow" case
  if (same_sign) {
    if ((carry_bit != 0 || is_negative()) && !rhs.is_negative()) {
      data_.push_back(carry_bit);
    }
    if (rhs.is_negative() && !is_negative()) {
      data_.push_back(LIMB_T_MAX);
    }
  }
}

big_integer& big_integer::operator+=(big_integer const& rhs)
{
  add_on_pref(rhs, 0);
  return *this;
}

big_integer& big_integer::operator-=(big_integer const& rhs)
{
  return *this += -rhs;
}

// ***multiplication***

big_integer& big_integer::operator*=(big_integer const& rhs)
{
  bool sign = is_negative() ^ rhs.is_negative();
  auto a = is_negative() ? negate() : *this;
  auto b(rhs);
  if (b.is_negative()) {
    b.negate();
  }

  b.normalize();
  a.normalize();
  *this = 0;
  new_buffer(a.len() + b.len() + 1);
  for (size_t i = 0; i < a.len(); i++) {
    limb_t carry_num = 0;
    for (size_t j = 0; j < b.len(); j++) {
      auto t1 = mul_limb_t(a.data_[i], b.data_[j]);
      auto t2 = data_[i + j];
      data_[i + j] += t1.second + carry_num;
      carry_num = t1.first + carry(carry_num, t2, t1.second);
    }
    data_[i + b.len()] += carry_num;
  }
  normalize();
  if (is_negative()) {
    data_.push_back(0);
  }
  return sign ? negate() : *this;
}

// ***division***

namespace {
  dlimb_t glue(limb_t x1, limb_t x0) {
    return (static_cast<dlimb_t>(x1) << LIMB_T_BITS) | x0;
  }
}

// source: www.youtube.com/channel/UCzymTLlAXdcgRn6yLcpow0Q
limb_t get_approx(big_integer a, big_integer b) {
  limb_t u2 = a.data_[2], u1 = a.data_[1], u0 = a.data_[0];
  limb_t d1 = b.data_[1], d0 = b.data_[0];
  dlimb_t U = glue(u2, u1);
  dlimb_t D = glue(d1, d0);
  if (u2 == d1) {
    dlimb_t S = glue(d0 - u1, 0) - u0;
    return S <= D ? LIMB_T_MAX : LIMB_T_MAX - 1;
  }
  dlimb_t Q = U / d1;
  dlimb_t DQ = Q * d0;
  dlimb_t R = glue(U - Q*d1, u0);
  if (R < DQ) {
    Q--, R += D;
    if (R >= D && R < DQ) {
      Q--, R += D;
    }
  }
  return Q;
}

void big_integer::mul_short(limb_t short_factor) {  // slightly faster version of `*=` for division
  limb_t carry_num = 0;
  for (size_t j = 0; j < len(); j++) {
    auto t = mul_limb_t(short_factor, data_[j]);
    data_[j] = t.second + carry_num;
    carry_num = t.first + carry(carry_num, t.second);
  }
  if (carry_num != 0) {
    data_.push_back(carry_num);
  }
}

big_integer big_integer::prefix(size_t pref_len) {
  big_integer ans;
  ans.new_buffer(pref_len);  // excess will be filled with 0-s
  for (size_t i = 0; i < pref_len; i++) {
    ans.data_[i] = len() < pref_len - i ? 0 : data_[len() - pref_len + i];
  }
  return ans;
}

void big_integer::assign_pref(big_integer const &rhs) {
  for (size_t i = 0; i < rhs.len(); i++) {
    data_[len() - rhs.len() + i] = rhs.data_[i];
  }
}

namespace {
  const int GREATER = 1;
  const int EQUAL = 0;
  const int LESS = -1;
}

int big_integer::compare_lexicographically(big_integer const &rhs, limb_t fill_value) const {
  for (size_t i = std::max(len(), rhs.len()); i --> 0;) {
    limb_t a = i < len() ? data_[i] : fill_value;
    limb_t b = i < rhs.len() ? rhs.data_[i] : fill_value;
    if (a == b) {
      continue;
    }
    return a > b ? 1 : -1;
  }
  return 0;
}

big_integer& big_integer::operator/=(big_integer const& rhs)
{
  error(rhs.is_zero(), "division by zero");
  bool sign = is_negative() ^ rhs.is_negative();
  if (is_negative()) {
    negate();
  }
  big_integer divisor(rhs);
  if (divisor.is_negative()) {
    divisor.negate();
  }
  if (divisor.compare_lexicographically(*this) == GREATER) {
    return *this = 0;
  }

  divisor.normalize();
  limb_t last_digit = divisor.data_.back();
  limb_t scaling_factor = last_digit == LIMB_T_MAX ? 1 : LIMB_T_MAX / (last_digit + 1);
  if (last_digit != LIMB_T_MAX
      && scaling_factor != LIMB_T_MAX
      && LIMB_T_MAX - last_digit >= scaling_factor * (last_digit + 1)) {
    scaling_factor++;
  }
  divisor.mul_short(scaling_factor);
  mul_short(scaling_factor);
  normalize();

  big_integer quot;
  quot.new_buffer(len() - divisor.len() + 1);
  data_.push_back(0);
  for (size_t i = quot.len(); i --> 0 ;) {
    limb_t qt = get_approx(prefix(3), divisor.prefix(2));
    big_integer pref(prefix(divisor.len() + 1));
    auto res(divisor);
    res.mul_short(qt);
    if (res.is_negative()) {
      res.data_.push_back(0);
    }
    if (res.compare_lexicographically(pref) == GREATER) {
      qt--;
      // Making `divisor` positive for valid subtraction (by adding a zero to the end)
      divisor.data_.push_back(0);
      res -= divisor;
      divisor.data_.pop_back();
    }
    quot.data_[i] = qt;
    pref.data_.push_back(0);  // just in case `pref` is negative
    pref -= res;
    pref.new_buffer(divisor.len() + 1);
    assign_pref(pref);
    if (len() > 1) {
      new_buffer(len() - 1);
    }
  }
  quot.normalize();
  if (quot.is_negative()) {
    quot.data_.push_back(0);
  }
  return *this = sign ? quot.negate() : quot;
}

big_integer& big_integer::operator%=(big_integer const& rhs)
{
  error(rhs.is_zero(), "division by zero");
  big_integer temp(*this);
  return *this -= (temp /= rhs) *= rhs;
}

// ***bitwise operations***

big_integer& big_integer::bit_operation(
        big_integer const &rhs,
        std::function<limb_t(limb_t, limb_t)> f) {
  new_buffer(std::max(len(), rhs.len()));
  auto rbr = rhs.rest_bits();
  for (size_t i = 0; i < len(); i++) {
    data_[i] = f(data_[i], i < rhs.len() ? rhs.data_[i] : rbr);
  }
  return *this;
}

big_integer& big_integer::operator&=(big_integer const& rhs)
{
  return bit_operation(rhs, std::bit_and<limb_t>());
}

big_integer& big_integer::operator|=(big_integer const& rhs)
{
  return bit_operation(rhs, std::bit_or<limb_t>());
}

big_integer& big_integer::operator^=(big_integer const& rhs)
{
  return bit_operation(rhs, std::bit_xor<limb_t>());
}

big_integer& big_integer::operator<<=(int rhs)
{
  if (rhs < 0) {
    return *this >>= -rhs;
  }
  auto fill_value = rest_bits();
  bool was_negative = is_negative();
  auto num = static_cast<size_t>(rhs);
  auto rest = num % LIMB_T_BITS;
  num /= LIMB_T_BITS;
  new_buffer(len() + num);
  for (size_t i = len(); i --> 0;) {
    data_[i] = i < num ? 0 : data_[i - num];
  }
  limb_t rem_bits = 0;
  for (size_t i = num; i < len() && rest > 0; i++) {
    auto new_rem_bits = rest == 0 ? 0 : data_[i] >> (LIMB_T_BITS - rest);
    data_[i] = (data_[i] << rest) | rem_bits;
    rem_bits = new_rem_bits;
  }
  if ((!was_negative && (is_negative() || rem_bits != 0))
      || (was_negative && rem_bits != 0 && (!is_negative() || pow2(rest) != rem_bits + 1))) {
    data_.push_back((fill_value << rest) | rem_bits);
  }
  return *this;
}

big_integer& big_integer::operator>>=(int rhs)
{
  if (rhs < 0) {
    return *this <<= -rhs;
  }
  auto num = std::min(static_cast<size_t>(rhs), LIMB_T_BITS * len());
  auto rest = num % LIMB_T_BITS;
  num /= LIMB_T_BITS;
  auto fill_value = rest_bits();
  for (size_t i = 0; i < len(); i++) {
    data_[i] = i < len() - num ? data_[i + num] : fill_value;
  }
  limb_t rem_bits = rest == 0 ? 0 : fill_value << (LIMB_T_BITS - rest);
  for (size_t i = len(); i --> 0 && rest > 0;) {
    auto new_rem_bits = data_[i] << (LIMB_T_BITS - rest);
    data_[i] = rem_bits | (data_[i] >> rest);
    rem_bits = new_rem_bits;
  }
  return *this;
}

// ***unary arithmetic operations***

big_integer big_integer::operator+() const
{
  return *this;
}

big_integer& big_integer::negate() {
  for (size_t i = 0; i < len(); i++) {
    data_[i] = ~data_[i];
  }
  limb_t carry_bit = 1;
  // *special case for 10...0*
  limb_t check_mask = 0;
  for (size_t i = 0; i < len() - 1; i++) {
    check_mask |= data_[i];
  }
  if (~data_[len() - 1] == pow2(LIMB_T_BITS - 1) && check_mask == 0) {
    new_buffer(len() + 1);
    return ++*this;
  }
  // *end of check*
  for (size_t i = 0; i < len() && carry_bit == 1; i++) {
    auto new_carry = carry(data_[i], carry_bit);
    data_[i]++;
    carry_bit = new_carry;
  }
  return *this;
}

big_integer big_integer::operator-() const
{
  big_integer r(*this);
  return r.negate();
}

big_integer big_integer::operator~() const
{
  big_integer r(*this);
  return r.bit_operation(-1, std::bit_xor<limb_t>());
}

big_integer& big_integer::operator++()
{
  return *this += 1;
}

big_integer big_integer::operator++(int)
{
  big_integer r(*this);
  ++*this;
  return r;
}

big_integer& big_integer::operator--()
{
  return *this += -1;
}

big_integer big_integer::operator--(int)
{
  big_integer r(*this);
  --*this;
  return r;
}

// ***binary operators***

big_integer operator+(big_integer a, big_integer const& b)
{
  return a += b;
}

big_integer operator-(big_integer a, big_integer const& b)
{
  return a -= b;
}

big_integer operator*(big_integer a, big_integer const& b)
{
  return a *= b;
}

big_integer operator/(big_integer a, big_integer const& b)
{
  return a /= b;
}

big_integer operator%(big_integer a, big_integer const& b)
{
  return a %= b;
}

big_integer operator&(big_integer a, big_integer const& b)
{
  return a &= b;
}

big_integer operator|(big_integer a, big_integer const& b)
{
  return a |= b;
}

big_integer operator^(big_integer a, big_integer const& b)
{
  return a ^= b;
}

big_integer operator<<(big_integer a, int b)
{
  return a <<= b;
}

big_integer operator>>(big_integer a, int b)
{
  return a >>= b;
}

// ***comparison***

int big_integer::compare_numerically(big_integer const &rhs) const {
  if (is_negative() && !rhs.is_negative()) {
    return LESS;
  } else if (!is_negative() && rhs.is_negative()) {
    return GREATER;
  }
  return compare_lexicographically(rhs, is_negative() ? LIMB_T_MAX : 0);
}

bool operator==(big_integer const& a, big_integer const& b)
{
  return a.compare_numerically(b) == EQUAL;
}

bool operator!=(big_integer const& a, big_integer const& b)
{
  return !(a == b);
}

bool operator<(big_integer const& a, big_integer const& b)
{
  return a.compare_numerically(b) == LESS;
}

bool operator>(big_integer const& a, big_integer const& b)
{
  return b < a;
}

bool operator<=(big_integer const& a, big_integer const& b)
{
  return a.compare_numerically(b) <= EQUAL;
}

bool operator>=(big_integer const& a, big_integer const& b)
{
  return !(a < b);
}

std::string to_string(big_integer const& a)
{
  if (a.is_zero()) {
    return "0";
  }
  std::string res;
  auto temp(a);
  if (temp.is_negative()) {
    temp.negate();
  }
  while (!temp.is_zero()) {
    auto digit = temp % MOD;
    temp /= MOD;
    auto digit_str = std::to_string(digit.data_[0]);
    res.append(digit_str.rbegin(), digit_str.rend());
    if (!temp.is_zero()) {
      res.append(DECIMAL_DIGIT_LEN - digit_str.size(), '0');
    }
  }
  if (a.is_negative()) {
    res.push_back('-');
  }
  std::reverse(res.begin(), res.end());
  return res;
}

std::ostream& operator<<(std::ostream& s, big_integer const& a)
{
  return s << to_string(a);
}
