---
layout: post
title:  X(旧Twitter)に投稿した小ネタ
date:   2023-10-02 11:10:00 +0900
---
小ネタ集です。

## Alembic
- たまたま見つけた[ブログ](https://i-saint.hatenablog.com/entry/2016/02/09/215542)に詳しく書かれてあった。
- [初期のドキュメント](https://code.google.com/archive/p/alembic/wikis/AlembicPoint9UsersGuide.wiki)

### Blenderで出力した場合
- 速度は出力されない
- 剛体運動はXformSampleに記録される
- ソフトボディの運動はXformSampleに記録されない
- ソフトボディと剛体がある時、例え剛体であっても各フレームごとのポリゴンメッシュの情報(点、法線、面)が出力される。


## CUDA
- CMakeによるコンパイル時に
```
In function `__sti____cudaRegisterAll()' * undefined reference to `__cudaRegisterLinkedBinary *
```
とエラーが出た時、`CUDA_SEPARABLE_COMPILATION`プロパティを実行ファイルにのみ付けるとエラーが治る。

- CMakeを使う場合、`project`の前に`CMAKE_CUDA_ARCHITECHTURES`を設定しなければならない。

### Thrust
- oneTBBをデバイスとして使う場合、コンパイルオプション`-expt-extended-lambda`をnvccに渡してもラムダ式をデバイスコードで使うことはできない。
- 非同期処理はCUDAとC++14以上が必要。CUDA以外に対しては実装されていない。
- 非同期処理を行うにはデータ型が[自明に再配置可能(trivially relocatable)](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r8.html)でなければならない。
- 複数のイベントの完了を待つイベントを作成するには`thrust::when_all`関数に完了を待ちたいイベントを渡せば良い。戻り値のイベントは`thrust::device_event`。
- 入れ子になった`host_vector`と`device_vector`はお互いにコピーできない(コンパイルエラーになる)。
- CMakeと一緒に使う時は[このドキュメント](https://github.com/NVIDIA/thrust/blob/main/thrust/cmake/README.md)を参考にしたほうがいい。

## ROCm
- ROCmをラップしたThrustを提供している。現時点でThrust 1.11まで対応しているため、非同期処理も行える。

## oneTBB
- FlowGraphは`async_node`が現時点でプレビューなので、非同期な処理を書くのに向いてない。

## C++
- [C++標準async+futureとスレッドプールの性能比較](https://mikio.hatenablog.com/entry/2021/07/13/224907)
- ユーザー定義とdefault宣言で定義されたコピーコンストラクタをマクロで切り替えることはできない。
- `struct A final {}`は合法。
- ラムダ関数は静的な変数を自動的にキャプチャする。
- 以下のような再帰的な構造体は合法。
```
template <int D>
struct A
{
    static_assert(D > 1);

    double d;
    A<D-1> a;
};

template <>
struct A<1>
{
    int d;
};
```
- テンプレートクラスの静的なメンバ定数は`inline`にすると明示的に実体を定義せずに済む。
- メンバ変数に`const`を付けると`default`宣言されたムーブ代入演算子が削除される。

### Boost
- Boost.TestをCMakeの`find_package`で見つけたい時は、`components`に`unit_test_framework`を指定しないといけない。


## Git
- git push --force-with-lease
- `git rm`でサブモジュールを消せる。
- fast-forwardでのマージを取り消したい時は、`git reflog --keep (ブランチ名)@{番号}`とすれば良い。
- 最新のコミットしか要らないリポジトリをクローンしたい時、`--depth 1`をつければ良い。
- 追跡していないファイルを消したい時は`git clean`。
- 追跡しているファイルを全てステージングしたい時は`git add -u`。


## 参考
1. [Thrust 1.9.4 リリースノート](https://github.com/NVIDIA/thrust/releases/tag/1.9.4)
1. [Predefined Node Types (oneTBB Documentation)](https://oneapi-src.github.io/oneTBB/main/tbb_userguide/Predefined_Node_Types.html)
1. [Undo git fast forward merge](https://stackoverflow.com/questions/17041317/undo-git-fast-forward-merge)
1. [テンプレート変数がラムダ関数にキャプチャされることによるエラーの例](https://wandbox.org/permlink/NiQFySYiXqODLaVr)
1. [再帰的な構造体の例](https://wandbox.org/permlink/VXCWD2IQlLIdz5wT)