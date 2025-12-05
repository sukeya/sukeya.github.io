# Expression templatesのダングリング対策について

本記事では、expression templatesの概要を述べ、欠点の一つであるダングリングする可能性に対処する方法を示します。

## Expression templatesとは
数値線形代数で最初に使われた手法で、構文木をテンプレートを使って擬似的に作ります。
利点は、一時変数の作成コストを削減できる点です。

例えば、3次元ベクトル`a, b, c`に対して、

```
Vec3 a, b, c;
sum = a + b + c;
```

を計算しようとすると、まず`tmp = a + b`が計算され、次に`sum = tmp + c`が計算されるでしょう。
Expression templatesでは、この計算を

```
for (int i = 0; i < 3; ++i) {
  sum[i] = a[i] + b[i] + c[i];
}
```

のように一括で行うことで一時変数を作らなくて済むようにします[^1]。
実際に、[Eigen](https://gitlab.com/libeigen/eigen)で使われています。

欠点は、コンパイル時のエラーメッセージが表現が複雑になるほど読みにくくなることとダングリングする可能性があることです。

## Expression templatesの実装例
Expression templatesの実装例として、実際に私が書いたコードの一部を抜粋し、改変しました。

```cpp
template <class L, class Op, class R>
class BinaryExpr final {
 public:
  static_assert(std::is_same_v<typename L::Domain, typename R::Domain>, "Domain mismatch!");
  static_assert(std::is_same_v<typename L::Range, typename R::Range>, "Range mismatch!");

  using Domain = typename L::Domain;
  using Range  = typename L::Range;

  BinaryExpr() = delete;

  BinaryExpr(const BinaryExpr&)            = default;
  BinaryExpr(BinaryExpr&&)                 = default;
  BinaryExpr& operator=(const BinaryExpr&) = default;
  BinaryExpr& operator=(BinaryExpr&&)      = default;
  ~BinaryExpr()                            = default;

  BinaryExpr(const L& l, const R& r) : l_(l), r_(r) {}

  // Calculate x of this
  Range operator()(const Domain& x) const { return Op::Apply(l_(x), r_(x)); }

  const auto& read_l() const { return l_; }
  const auto& read_r() const { return r_; }

 private:
  const L& l_;
  const R& r_;
};

struct Plus final {
  template <class R>
  requires(!std::is_floating_point_v<R>)
  static R Apply(R&& l, R&& r) {
    return std::forward<R>(l) + std::forward<R>(r);
  }

  template <class R>
  requires std::is_floating_point_v<R>
  static R Apply(R l, R r) {
    return l + r;
  }
};

struct Multiply final {
  template <class R>
  requires(!std::is_floating_point_v<R>)
  static R Apply(R&& l, R&& r) {
    return std::forward<R>(l) * std::forward<R>(r);
  }

  template <class R>
  requires std::is_floating_point_v<R>
  static R Apply(R l, R r) {
    return l * r;
  }
};
```

`BinaryExpr`のテンプレート引数`Op`は二項演算を表し、`Plus`や`Multiply`が代入されることを想定しています。
`L`と`R`は関数オブジェクトを想定しています。
そして、`operator()`で受け取った引数`x`を`l`と`r`に渡して、`Op::Apply`関数が呼び出し、計算結果が返されます。

ついでに、関数クラスも作りましょう。

```cpp
class F {
public:
    using Domain = double;
    using Range = double;

    template <class T>
    F(T&& f) : f_(f) {}

    double operator()(double x) const {
        return f_(x);
    }

private:
    std::function<double(double)> f_;
};
```

この実装例は大体うまく行きます。

`l`または`r`が一時オブジェクトじゃなければ。

## ダングリングへの対処法
C++では、「一時オブジェクト`tmp`を`const&`の変数`x`に代入すると、`tmp`の寿命が`x`の寿命に延長される」という仕様があります。
しかし、この`x`を別の`const&`の変数`y`に代入しても、`tmp`の寿命は`x`の寿命と同じままです[^2]。

そのため、以下のようなコードはダングリングポインタを発生させます。

```cpp
BinaryExpr<F, Plus, F>{
  [](double x){ return x + 1; },
  [](double x){ return x * x; }
}
```

では、どうするか？

私が考えた対処法は演算対象の型(`L`と`R`)のラッパークラスを作成し、演算対象の型が`const&`があるか`&&`があるかによって、ラッパークラスの実装を切り変えるという方法です。

具体的には、以下の型を作ります。

```cpp
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
```

大まかに解説すると、`const&`と`&&`を判別するコンパイル時定数を作成し、`const&`ならそのまま、`&&`ならムーブして値を保持するクラスが`RefWrapper`です。
値をムーブできるように`move`メンバ関数を用意しています。

これを先ほどの`BinaryExpr`に組み込むと以下のようになります:

```cpp title="src/expr_template/example.cpp" linenums="1"
--8<-- "./src/expr_template/example.cpp"
```

実行結果は[こちら](https://godbolt.org/z/Kqn8KxqYn)。

### 課題
ダングリングの問題は対処できました。
では、拡張性についてはどうでしょうか？

例えば、微分したいとしましょう。
数学を学んだ者の端くれとしては関数として実装したい所です。
すると、以下の場合分けが発生します:

1. `BinaryExpr`が右辺値かどうか
2. `L`が右辺値かどうか
3. `R`が右辺値かどうか
4. `Op`の種類

実際は、`BinaryExpr`が`const&`ならば、2と3の場合分けは無視できますし、対称性を使えばコード量は減るでしょう。
それでも、8種類の部分特殊化と`F`への微分の実装が必要になってしまいます。

残念ながら、関数として実装するのは大変です。

メンバ関数として実装するのが良いのかもしれませんが、試せてません。
(`Eigen`の実装を考えるとそう思えますが)

## まとめ
Expression templatesの概要を述べ、欠点の一つであるダングリングする可能性に対処する方法を示しました。
私の方法では、Expression templatesを受け取る関数を実装するのは大変なので、設計段階でどんなメンバ関数が必要かあらかじめ決めておくと良いのかもしれません。

## 参考文献と謝辞
[^1]: 『C++テンプレートテクニック 第２版』(著: エピステーメー, 高橋 晶)
[^2]: [キノコになりたい](https://x.com/onihusube9)さんからご回答いただきました。ありがとうございました。
