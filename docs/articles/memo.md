# メモ
小ネタ集です。


## BlenderでAlembicを出力した場合
- 速度は出力されない
- 剛体運動はXformSampleに記録される
- ソフトボディの運動はXformSampleに記録されない
- ソフトボディと剛体がある時、例え剛体であっても各フレームごとのポリゴンメッシュの情報(点、法線、面)が出力される。


## Thrust
- oneTBBをデバイスとして使う場合、コンパイルオプション`-expt-extended-lambda`をnvccに渡してもラムダ式をデバイスコードで使うことはできない。
- 入れ子になった`host_vector`と`device_vector`はお互いにコピーできない(コンパイルエラーになる)。


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


## PETc
### PETScのC++の機能を使うには
PETScには`PetscCallThrow`という、エラーが発生すると`std::runtime_error`を投げる関数がある。
この関数は`PETSC_CLANGUAGE_CXX`というマクロが定義されていないと使えない。
他にもC++用の関数があるかもしれないので定義することをお勧めする。
ちなみに、ヘッダーをインクルードする時は`extern "C"`をつけない。

### PETScのconfigureの実行オプション
[公式サイト](https://petsc.org/release/install/install/)に用途に応じて使うオプションが示されているが、どんなオプションがあるのか示されていない。
どんなオプションがあるのか調べるための一番手っ取り早い方法は、[CIのテスト](https://gitlab.com/petsc/petsc/-/tree/main/config/examples)を見ることである。
地道な方法はソースコードを見ることだが、`BuildSystem`があまりに複雑なので時間がかかるだろう。
個人的には[package](https://gitlab.com/petsc/petsc/-/tree/main/config/BuildSystem/config/packages)を見ると、ライブラリ毎のオプションがどういう風に使われているかわかるので、ここを見ることをお勧めする。


## 参考
1. [Thrust 1.9.4 リリースノート](https://github.com/NVIDIA/thrust/releases/tag/1.9.4)
1. [Predefined Node Types (oneTBB Documentation)](https://oneapi-src.github.io/oneTBB/main/tbb_userguide/Predefined_Node_Types.html)
1. [Undo git fast forward merge](https://stackoverflow.com/questions/17041317/undo-git-fast-forward-merge)
1. [テンプレート変数がラムダ関数にキャプチャされることによるエラーの例](https://wandbox.org/permlink/NiQFySYiXqODLaVr)
1. [再帰的な構造体の例](https://wandbox.org/permlink/VXCWD2IQlLIdz5wT)