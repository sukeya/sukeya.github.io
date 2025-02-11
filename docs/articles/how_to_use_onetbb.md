# oneTBBの使い方

C++ Advent Calendar 2024 22日目の記事です。
今回は意外と少なかったoneTBB(旧intel TBB)の使い方について書きます。


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

```cpp title="oneapi/tbb/blocked_range.h"
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

    `begin`の戻り値はイテレータではなく`Value`型の値です。
    間違えやすい上によく使うので、注意しましょう。

以下の関数では、渡された`Range`型のインスタンスを分割して、各スレッドで分割後の範囲を実行しています。


### `parallel_for`
```cpp title="oneapi/tbb/parallel_for.h"
namespace oneapi::tbb {
    template<typename Range, typename Body>
    void parallel_for(const Range& range, const Body& body);
}
```

for文のように、各要素に対して`body`を並列に実行する関数です。
`Body`型にも要件がありますが[^5]、`[...](const Range& r) {...}`で十分です。

以下は`parallel_for`を使った簡単な例です。

```cpp title="src/how_to_use_onetbb/parallel_for.cpp" linenums="1"
--8<-- "./src/how_to_use_onetbb/parallel_for.cpp"
```

!!! note

    `parallel_for`はループ全体で少なくとも100万命令以上ある処理に対して使いましょう。


### `parallel_reduce`
```cpp title="oneapi/tbb/parallel_reduce.h"
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
この節で登場する関数は一部シングルスレッドで動作します。
よく考えて使ってください。

### `parallel_for_each`
```cpp title="oneapi/tbb/parallel_for_each.h"
namespace oneapi::tbb {
    // (1)
    template<typename InputIterator, typename Body>
    void parallel_for_each( InputIterator first, InputIterator last, Body body );

    // (2)
    template<typename Container, typename Body>
    void parallel_for_each( Container& c, Body body );

    // (3)
    template<typename Container, typename Body>
    void parallel_for_each( const Container& c, Body body );
}
```

この関数はループ回数がわからないけど、各要素を並列処理したいという時に使います。
一見便利に見えますが、要素アクセスがシングルスレッドで動作するので注意しましょう。

テンプレートパラメーター`InputIterator`は[入力イテレータ](https://cpprefjp.github.io/reference/iterator/input_iterator.html)でなければいけません。
`body`は`InputIterator`が[前方向イテレータ](https://cpprefjp.github.io/reference/iterator/forward_iterator.html)かどうかでwell-definedな引数が決まります。[^15]
説明のため

```cpp
using value_type = typename std::iterator_traits<InputIterator>::value_type;
```

とすると、

1. `InputIterator`が前方向イテレータでない場合、以下のラムダ式を`body`に渡せます。
```cpp
[...](const value_type& item) {...}
[...](value_type&& item) {...}
```
2. `InputIterator`が前方向イテレータの場合、1に加えて以下のラムダ式も`body`に渡せます。
```cpp
[...](value_type& item) {...}
```

オーバーロードの2と3は`parallel_for_each(std::begin(c), std::end(c), body)`と同じです。

```cpp title="src/how_to_use_onetbb/parallel_for_each.cpp" linenums="1"
--8<-- "./src/how_to_use_onetbb/parallel_for_each.cpp"
```

#### ループ中に要素を追加したいとき
ループ中に要素を追加したいとき、以下のように`body`に引数を追加する必要があります。

```cpp
[...](ItemType item, oneapi::tbb::feeder<ItemType>& feeder) {...}
```

`ItemType`は上で説明した`value_type`のいずれかの参照型です。
ここで追加した`feeder`型の`add`メンバ関数を使って、ループに要素を追加します。[^14]

```cpp title="oneapi/tbb/parallel_for_each.h"
namespace oneapi::tbb {
    template<typename Item>
    class feeder {
    public:
        // (1)
        void add(const Item& item);
        // (2)
        void add(Item&& item);
    };
} //namespace oneapi::tbb
```

使うメンバ関数によって、`Item`型は以下の要件を満たす必要があります。

1. コピー構築可能
2. ムーブ構築可能

以下は、開始から1秒経過するまで何度もタイマーをコンテナに追加して寝続ける例です。

```cpp title="src/how_to_use_onetbb/parallel_for_each_with_feeder.cpp" linenums="1"
--8<-- "./src/how_to_use_onetbb/parallel_for_each_with_feeder.cpp"
```

### `parallel_pipeline`
```cpp title="oneapi/tbb/parallel_pipeline.h"
namespace oneapi::tbb {
    void parallel_pipeline(size_t max_number_of_live_tokens, const filter<void,void>& filter_chain);
}
```

この関数は工場の流れ作業のように、`filter_chain`に与えられた関数を実行します。
詳しい説明の前に具体例を見た方が早いでしょう。

```cpp title="src/how_to_use_onetbb/parallel_pipeline.cpp" linenums="1"
--8<-- "./src/how_to_use_onetbb/parallel_pipeline.cpp"
```

見ての通り、`filter_chain`には`oneapi::tbb::make_filter`[^8]で作成した`filter`を渡します。
別のフィルターを後ろにつけるには`&`で繋げます。

フィルター間で受け渡しされる値をトークンとすると、`max_number_of_live_tokens`は並列実行可能なトークンの最大数です[^11]。

#### `make_filter`
```cpp title="oneapi/tbb/parallel_pipeline.h"
namespace oneapi::tbb {
    template<typename InputType, typename OutputType, typename Body>
    filter<InputType, OutputType> make_filter(filter_mode mode, const Body& body);
}
```

`InputType`には前のフィルターから受け取る値の型を、`OutputType`には後ろのフィルターに渡す値の型を書きます。
ない場合は`void`です。

`mode`には以下の表の値を指定します。[^9]
すべて`oneapi::tbb`名前空間の下にあります。

|`mode`|意味|
|--|--|
| `parallel` | 並列実行 |
| `serial_in_order` | 順番通りに逐次実行 |
| `serial_out_of_order` | 順不同に逐次実行 |

`body`は最初のフィルターでない限り、`[...](InputType&&) -> OutputType {...}`を渡せば良いです(`body`には右辺値が渡されます[^10])。
最初のフィルターの場合、`[...](oneapi::tbb::flow_control& fc) -> OutputType {...}`を渡せば良いです。
ただし、関数内で`fc.stop()`を呼んでパイプラインを終わらせる必要があります。

## その他
### `parallel_sort`
```cpp title="oneapi/tbb/parallel_sort.h"
namespace oneapi::tbb {
    // (1)
    template<typename RandomAccessIterator>
    void parallel_sort(RandomAccessIterator begin, RandomAccessIterator end);

    // (2)
    template<typename RandomAccessIterator, typename Compare>
    void parallel_sort(RandomAccessIterator begin, RandomAccessIterator end, const Compare& comp);

    // (3)
    template <typename Container>
    void parallel_sort(Container&& c) {
        parallel_sort(std::begin(c), std::end(c));
    }

    // (4)
    template <typename Container, typename Compare>
    void parallel_sort(Container&& c, const Compare& comp) {
        parallel_sort(std::begin(c), std::end(c), comp);
    }
}
```

与えられた半開区間`[begin, end)`またはコンテナ`c`を、`std::less`または`comp`を用いて並列にソートする関数。
イテレータ`RandomAccessIterator`はランダムアクセスイテレータで、値はムーブ可能でなければならない。
また、比較オブジェクト`comp`は`bool operator()(const T&, const T&)`(`T`は値型)を実装していなければならない。

## FAQ
### Parallel STLがあるからTBB要らないんじゃない？
簡単な並列処理ならPSTLを使ったほうが楽だと思いますが、TBBを直に使う方がより複雑な並列処理ができます。
実装の詳細までは知りませんが、TBBではちゃんと設定しないと`parallel_for`の中で他の並列処理を呼ぶと正しく処理してくれません。なので、そういう処理を行う際は必要になるかなと思います。

あと説明していませんでしたが、TBBにはFlow Graphという命令並列な並列処理を行う機能があります。
また、タスクの中断時の挙動の設定やエラー処理もTBBにはあるので、こういう機能で差別化できているかなと思います。


## 余談
`parallel_reduce`の大量の`std::set`をマージするという例は実は私がコードを書いていた中で出てきた問題でした。
`oneTBB`にイシューを立てたところ、`Body`を使った方法を親切に教えてもらいました。
intelは最近あまり良い話題がありませんが、何らかの形で恩返し出来たらなと思います。


### PSTLの実装について
実はGNU、LLVMどちらも内部でTBBを使っています。
具体的には、GNUはLLVMの実装を使っていて、フラグでTBBを使うようにしており[^12]、
LLVMの方もデフォルトでは逐次実行ですが、TBBを使うフラグを用意しています[^13]。

LLVMの実装の理由は環境依存の依存関係を作りたくないからのようですが、使う際は逐次実行の可能性もあるため、どのようにビルドしたかまで調べる必要があります。
気持ちはわかるんですが、こんなことされるとPSTL使いたくないなと思ってしまいました。


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
[^8]: [make_filter](https://github.com/uxlfoundation/oneTBB/blob/master/include/oneapi/tbb/parallel_pipeline.h#L90)
[^9]: [filter_mode](https://github.com/uxlfoundation/oneTBB/blob/master/include/oneapi/tbb/parallel_pipeline.h#L38)
[^10]: [Bodyに右辺値が渡される箇所](https://github.com/uxlfoundation/oneTBB/blob/master/include/oneapi/tbb/detail/_pipeline_filters.h#L242)
[^11]: [現在のトークン数が最大数を超えてないか比較している箇所](https://github.com/uxlfoundation/oneTBB/blob/master/src/tbb/parallel_pipeline.cpp#L283)
[^12]: [GNU C++ ライブラリがPSTLの内部でTBBを使うようにフラグを設定している箇所](https://github.com/gcc-mirror/gcc/blob/81d4707a00a2d74a9caf2d806e5b0ebe13e1247c/libstdc%2B%2B-v3/include/bits/c%2B%2Bconfig#L927)
[^13]: [PSTL_PARALLEL_BACKENDで並列処理のライブラリを切り替えられます](https://github.com/llvm/llvm-project/blob/ddef380cd6c30668cc6f6d952b4c045f724f8d57/pstl/CMakeLists.txt#L23)。[議論の中にも出てきました](https://discourse.llvm.org/t/parallel-stl/56381/2)。
[^14]: [parallel_for_eachで使われるfeeder](https://oneapi-spec.uxlfoundation.org/specifications/oneapi/latest/elements/onetbb/source/algorithms/functions/feeder)
[^15]: [parallel_for_each Body semantics and requirements](https://uxlfoundation.github.io/oneTBB/main/reference/parallel_for_each_semantics.html)