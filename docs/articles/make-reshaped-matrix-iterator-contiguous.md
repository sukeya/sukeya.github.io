# `Eigen`で行優先の行列を行優先のベクトルに変換する時、ベクトルのイテレータを`std::contiguous_iterator`にする方法

この記事は私が`Eigen`に立てた[イシュー](https://gitlab.com/libeigen/eigen/-/issues/2840)が解決される見込みがないので、埋もれる前に残しておくつもりで書きました。

## 問題点
`Eigen`では行列と配列のイテレータが`std::contiguous_iterator`コンセプトを満たすようになりました[^1][^2][^3]。
ただし、とある条件を満たす必要があり、`reshaped`メンバ関数で行列をベクトルに変換する場合は変形後のベクトルと変形前の行列の格納順が一致しなくてはいけません。
`reshaped`で返されるベクトルの格納順はそのベクトルが行ベクトルなら行優先、列ベクトルなら列優先です[^5]。
しかし、変形後のベクトルの格納順はたとえ明示的に`Eigen::StorageOptions::RowMajor`を渡しても必ず列ベクトルを返すため[^6]、変形前の行列が行優先の場合だと不一致になってしまいます。

```cpp
#include <span>

#include "Eigen/Dense"

int main() {
    Eigen::Matrix3f m;
    std::span<const float, 9>(m.reshaped<Eigen::StorageOptions::RowMajor>().begin(), 9); // エラー
    return 0;
}
```

## 解決策
`Eigen`には`Eigen::fix`という、関数によって構築されるオブジェクトの整数テンプレート引数にコンパイル時整数を指定することができる関数が用意されています[^4]。
この関数を使って、以下のように行数と列数を指定すると変形後のベクトルは行ベクトルとなり、行優先の行列を変形しても格納順が一致します。

```cpp
#include <span>

#include "Eigen/Dense"

int main() {
    Eigen::Matrix3f m;
    std::span<const float, 9>(
        m.reshaped<Eigen::StorageOptions::RowMajor>(Eigen::fix<1>, Eigen::fix<9>).begin(), 9);
    return 0;
}
```

## 参考
[^1]: [allow pointer_based_stl_iterator to conform to the contiguous_iterator concept if we are in c++20](https://gitlab.com/libeigen/eigen/-/merge_requests/1636)
[^2]: [Eigen/src/Core/Matrix.h](https://gitlab.com/libeigen/eigen/-/blob/master/Eigen/src/Core/Matrix.h#L50)
[^3]: [Eigen/src/Core/Array.h](https://gitlab.com/libeigen/eigen/-/blob/master/Eigen/src/Core/Array.h#L20)
[^4]: [Eigen::fix](https://libeigen.gitlab.io/docs/group__Core__Module.html#gac01f234bce100e39e6928fdc470e5194)
[^5]: [Eigen/src/Core/Reshaped](https://gitlab.com/libeigen/eigen/-/blob/master/Eigen/src/Core/Reshaped.h#L63)
[^6]: [Eigen/src/plugins/ReshapedMethods.inc](https://gitlab.com/libeigen/eigen/-/blob/master/Eigen/src/plugins/ReshapedMethods.inc#L117)
