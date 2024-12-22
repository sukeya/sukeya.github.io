# oneTBBの使い方

C++ Advent Calendar 2024 22日目の記事です。
今回は意外と少なかったoneTBB(旧intel TBB)の使い方について書きます。

書ききれていない部分は年内に書こうと思います。

## oneTBBとは
[oneTBB](https://github.com/uxlfoundation/oneTBB)とはintelが中心となって開発している、お手軽マルチスレッディングライブラリです。
C++20ではセマフォやバリアといった多数の同期処理に関する機能が追加されましたが、これらを使ってマルチスレッドなプログラムを書くのは慣れてないとなかなか大変だと思います。
oneTBBでは`for`文や`reduce`、`sort`といった処理を並列処理する関数やスレッドセーフなコンテナを用意しており、並列処理に詳しくなくても書けるように設計されています。

名前空間は`oneapi::tbb`で、`oneapi/tbb.h`をインクルードすれば全機能が使えます。

では、実際にどんな関数があるのか見ていきましょう。


## 単純な並列処理
ここでは、比較的単純な並列処理を行う関数について見ていきます。


### 前提知識
以下で紹介する関数は引数に`Range`型という、何らかの範囲を表すクラスのインスタンスを指定します。

??? note

    `parallel_for`には指定しない関数オーバーロードもありますが、並列実行されるとは限らないため省略します[^1]。

これは例えば

$$
\sum_{n = 1}^{\infty} \frac{1}{n^2}
$$

の計算のように、各要素の計算は時間がかからないが要素数が多い場合、スレッド間の同期処理によるオーバーヘッドを減らすために導入されています。

いくつか種類がありますが[^3]、`blocked_range`を知っていれば大体のケースは問題ないでしょう。

```cpp
namespace oneapi::tbb {
template<typename Value>
class blocked_range {
    //! Type of a value
    /** Called a const_iterator for sake of algorithms that need to treat a blocked_range as an STL container. */
    using const_iterator = Value;

    //! Type for size of a range
    using size_type = std::size_t;

    //! Construct range over half-open interval [begin,end), with the given grainsize.
    blocked_range( Value begin_, Value end_, size_type grainsize_=1 );

    //! Beginning of range.
    const_iterator begin() const;

    //! One past last value in range.
    const_iterator end() const;

    //! Size of the range
    size_type size() const;

    //! True if range is empty.
    bool empty() const;
};
}
```

ここでは、ユーザーが知っていればいいものだけを抜き出しました。
このクラス[^2]は0以上の整数の範囲 (正確には右半開区間) を表すために使われます。
テンプレートパラメーターの`Value`は普通は`std::size_t`を指定すれば良いです[^4]。
コンストラクタで範囲を指定しましょう。

!!! note

    気をつける点は`begin`がイテレータではなく`Value`型の値を返すことです。
    間違えやすい上によく使うので、注意しましょう。

以下の関数では、渡された`Range`型のインスタンスを分割して、各スレッドで分割後の範囲を実行しています。


### `parallel_for`
```cpp
namespace oneapi::tbb {
  template<typename Range, typename Body>
  void parallel_for(const Range& range, const Body& body);
}
```

for文のように、各要素に対して`body`を並列に実行する関数です。
`Body`型にも要件がありますが[^5]、`[...](const Range& r) -> void {...}`で十分です。

以下は`parallel_for`を使った簡単な例です。

```cpp title="src/how_to_use_onetbb/parallel_for.cpp" linenums="1"
--8<-- "./src/how_to_use_onetbb/parallel_for.cpp"
```

!!! note

    `parallel_for`はループ全体で少なくとも100万命令以上ある処理に対して使いましょう。


### `parallel_reduce`
```cpp
namespace oneapi::tbb {
  template<typename Range, typename Value, typename Func, typename Reduction>
  Value parallel_reduce(const Range& range, const Value& identity, const Func& func, const Reduction& reduction);
}
```

`parallel_reduce`[^6]は`std::reduce`のように、与えられた範囲`range`を集計する関数です。集計順は

`range`以外の仮引数の要件は、**単純化すると**以下の通りです。

- `identity`: コピー構築可かつコピー代入可な型の左単位元(`x == reduction(identity, x)`を満たす値)。`func`の仮引数`x`に渡されます。
- `func`: `[...](const Range& r, const Value& x) -> Value {...}`を渡せば十分。この関数は初期値`x`に`r`の範囲の値を集計するために使われます。
- `reduction`: `[...](const Value& x, const Value& y) -> Value {...}`を渡せば十分。この関数は2つの値をまとめるために使われます。

以下は`parallel_reduce`を使った簡単な例です。

```cpp title="src/how_to_use_onetbb/parallel_reduce.cpp" linenums="1"
--8<-- "./src/how_to_use_onetbb/parallel_reduce.cpp"
```

多数の`std::set`をマージすることも出来ます。[^7]
**実行していないので、間違っているかもしれません。**

```cpp title="src/how_to_use_onetbb/parallel_reduce_rvalue.cpp" linenums="1"
--8<-- "./src/how_to_use_onetbb/parallel_reduce_rvalue.cpp"
```

??? note

    `parallel_reduce`には実はもう一つ
    ```
    template<typename Range, typename Body>
    void parallel_reduce(const Range& range, Body& body);
    ```
    というオーバーロードがあります。このオーバーロードは2つ目の例のような右辺値を集計したい時に使う機会がありましたが、1つ目の方が対応したため使う機会がないと思い省略しました。


## 複雑な並列処理
年内に書きます。

### `parallel_for_each`
### `parallel_pipeline`

## その他
年内に書きます。

### `parallel_sort`

## 余談
`parallel_reduce`の大量の`std::set`をマージするという例は実は私がコードを書いていた中で出てきた問題でした。
`oneTBB`にイシューを立てたところ、`Body`を使った方法を親切に教えてもらいました。
intelは最近あまり良い話題がありませんが、何らかの形で恩返し出来たらなと思います。

## 参考
- [oneAPI Threading Building Blocks (oneTBB)](https://uxlfoundation.github.io/oneTBB/index.html)
- [oneTBB](https://github.com/uxlfoundation/oneTBB)

[^1]: [parallel_for](https://uxlfoundation.github.io/oneAPI-spec/spec/elements/oneTBB/source/algorithms/functions/parallel_for_func.html)
[^2]: [blocked_range](https://github.com/uxlfoundation/oneTBB/blob/master/include/oneapi/tbb/blocked_range.h#L44)
[^3]: [Advanced Topic: Other Kinds of Iteration Spaces](https://uxlfoundation.github.io/oneTBB/main/tbb_userguide/Advanced_Topic_Other_Kinds_of_Iteration_Spaces.html)
[^4]: [blocked_range_value](https://github.com/uxlfoundation/oneTBB/blob/master/include/oneapi/tbb/detail/_range_common.h#L87)
[^5]: [Body requirements](https://github.com/uxlfoundation/oneTBB/blob/master/include/oneapi/tbb/parallel_for.h#L213)
[^6]: [parallel_reduce](https://uxlfoundation.github.io/oneAPI-spec/spec/elements/oneTBB/source/algorithms/functions/parallel_reduce_func.html)
[^7]: [Parallel Reduction for rvalues](https://uxlfoundation.github.io/oneTBB/main/reference/rvalue_reduce.html)
