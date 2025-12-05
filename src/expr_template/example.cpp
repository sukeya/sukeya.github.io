//  Copyright 2023,2025 Yuya Asano
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#include <concepts>
#include <functional>
#include <type_traits>

template <class T>
constexpr bool is_const_v = std::is_const_v<std::remove_reference_t<T>>;

template <class T>
constexpr bool is_const_ref_v = is_const_v<T> && std::is_lvalue_reference_v<T>;

template <class T>
constexpr bool is_rvalue_v = std::is_rvalue_reference_v<T> && (!is_const_v<T>);

template <class T>
constexpr bool is_cref_or_rvalue_v = is_const_ref_v<T> || is_rvalue_v<T>;

template <class T>
struct FilterRvalue;

template <class T>
struct FilterRvalue<T&&> {
  using type = T;

  static constexpr bool is_rvalue = true;
};

template <class T>
struct FilterRvalue<const T&> {
  using type = const T&;

  static constexpr bool is_rvalue = false;
};

template <class T_>
requires is_cref_or_rvalue_v<T_>
class RefWrapper final {
  using FilterResult = FilterRvalue<T_>;

 public:
  using T = std::remove_cvref_t<T_>;

  using Domain = typename T::Domain;
  using Range  = typename T::Range;

  using Storage = FilterResult::type;

  static constexpr bool is_rvalue = FilterResult::is_rvalue;

  RefWrapper() = delete;

  RefWrapper(const RefWrapper&)            = default;
  RefWrapper(RefWrapper&&)                 = default;
  RefWrapper& operator=(const RefWrapper&) = default;
  RefWrapper& operator=(RefWrapper&&)      = default;
  ~RefWrapper()                            = default;

  RefWrapper(T_&& t) : t_(std::forward<T_>(t)) {}

  Range operator()(const Domain& x) const { return t_(x); }

  const Storage& read() const
  requires(!is_rvalue)
  {
    return t_;
  }

  const Storage& read() const
  requires(is_rvalue)
  {
    return t_;
  }

  const Storage& move() &&
  requires(!is_rvalue)
  {
    return t_;
  }

  Storage move() &&
  requires(is_rvalue)
  {
    return std::move(t_);
  }

 private:
  Storage t_;
};

// L and R may be either const& or &&.
template <class LRef_, class Op, class RRef_>
requires is_cref_or_rvalue_v<LRef_> && is_cref_or_rvalue_v<RRef_>
class BinaryExpr final {
  using LRef = RefWrapper<LRef_>;
  using RRef = RefWrapper<RRef_>;

 public:
  using L = std::remove_cvref_t<LRef_>;
  using R = std::remove_cvref_t<RRef_>;

  static_assert(std::is_same_v<typename L::Domain, typename R::Domain>, "Domain mismatch!");
  static_assert(std::is_same_v<typename L::Range, typename R::Range>, "Range mismatch!");

  using Domain = typename L::Domain;
  using Range  = typename L::Range;

  static constexpr bool is_l_rvalue = LRef::is_rvalue;
  static constexpr bool is_r_rvalue = RRef::is_rvalue;

  BinaryExpr() = delete;

  BinaryExpr(const BinaryExpr&)            = default;
  BinaryExpr(BinaryExpr&&)                 = default;
  BinaryExpr& operator=(const BinaryExpr&) = default;
  BinaryExpr& operator=(BinaryExpr&&)      = default;
  ~BinaryExpr()                            = default;

  BinaryExpr(LRef_&& l, RRef_&& r) : l_(std::forward<LRef_>(l)), r_(std::forward<RRef_>(r)) {}

  // Calculate x of this
  Range operator()(const Domain& x) const { return Op::Apply(l_(x), r_(x)); }

  const auto& read_l() const { return l_.read(); }

  const auto& read_r() const { return r_.read(); }

  auto move_l()
  requires(is_l_rvalue)
  {
    return std::move(l_).move();
  }

  decltype(auto) move_l()
  requires(!is_l_rvalue)
  {
    return std::move(l_).move();
  }

  auto move_r()
  requires(is_r_rvalue)
  {
    return std::move(r_).move();
  }

  decltype(auto) move_r()
  requires(!is_r_rvalue)
  {
    return std::move(r_).move();
  }

 private:
  LRef l_;
  RRef r_;
};

struct Plus {
  template <class R>
  requires std::is_floating_point_v<R>
  static R Apply(R l, R r) {
    return l + r;
  }
};

struct Multiply {
  template <class R>
  requires std::is_floating_point_v<R>
  static R Apply(R l, R r) {
    return l * r;
  }
};

class F {
public:
    using Domain = double;
    using Range = double;

    template <class T>
    F(T&& f) : f_(f) {}
    double operator()(double x) const {
        return f_(x);
    }

    friend auto operator+(const F& l, const F& r) {
        return BinaryExpr<const F&, Plus, const F&>(l, r);
    }

    friend auto operator+(F&& l, const F& r) {
        return BinaryExpr<F&&, Plus, const F&>(std::move(l), r);
    }

    friend auto operator+(const F& l, F&& r) {
        return BinaryExpr<const F&, Plus, F&&>(l, std::move(r));
    }

    friend auto operator+(F&& l, F&& r) {
        return BinaryExpr<F&&, Plus, F&&>(std::move(l), std::move(r));
    }

    friend auto operator*(const F& l, const F& r) {
        return BinaryExpr<const F&, Multiply, const F&>(l, r);
    }

    friend auto operator*(F&& l, const F& r) {
        return BinaryExpr<F&&, Multiply, const F&>(std::move(l), r);
    }

    friend auto operator*(const F& l, F&& r) {
        return BinaryExpr<const F&, Multiply, F&&>(l, std::move(r));
    }

    friend auto operator*(F&& l, F&& r) {
        return BinaryExpr<F&&, Multiply, F&&>(std::move(l), std::move(r));
    }

private:
    std::function<double(double)> f_;
};


int main() {
    // case 1
    {
        F f{[](double x) -> double {
            return 1 + x;
        }};
        F g{[](double x) -> double {
            return x * x;
        }};
        auto sum = f + g;

        if (sum(0.5) != 1.75) {
            return 1;
        }
    }

    // case 2
    {
        F f{[](double x) -> double {
            return 1 + x;
        }};

        auto sum = f + F{[](double x) -> double {
            return x * x;
        }};

        if (sum(0.5) != 1.75) {
            return 1;
        }
    }
    
    // case 3
    {
        auto sum = F{[](double x) -> double {
            return 1 + x;
        }} + F{[](double x) -> double {
            return x * x;
        }};

        if (sum(0.5) != 1.75) {
            return 1;
        }
    }

    // case 4
    {
        // (1 + x) * (1 + 2 * x) + x^2
        auto sum = F{[](double x) -> double {
            return 1 + x;
        }} * F{[](double x) -> double {
            return 1 + 2 * x;
        }} + F{[](double x) -> double {
            return x * x;
        }};

        if (sum(0.5) != 3.25) {
            return 1;
        }
    }
}
