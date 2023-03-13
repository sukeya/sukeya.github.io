---
layout: post
title:  PETScの使い方のコツ
date:   2023-03-13 10:20:00 +0900
---
[PETSc](https://petsc.org/release/)の使い方のコツを忘れないうちに書き留めておく。

## PETScのC++の機能を使うには
PETScには`PetscCallThrow`という、エラーが発生すると`std::runtime_error`を投げる関数がある。
この関数は`PETSC_CLANGUAGE_CXX`マクロが定義されていないと使えない。
他にもC++用の関数があるかもしれないので定義することをお勧めする。
ちなみに、ヘッダーをインクルードする時は`extern "C"`をつけない。

## PETScのconfigureの実行オプション
[公式サイト](https://petsc.org/release/install/install/)に用途に応じて使うオプションが示されているが、どんなオプションがあるのか示されていない。
どんなオプションがあるのか調べるための一番手っ取り早い方法は、[CIのテスト](https://gitlab.com/petsc/petsc/-/tree/main/config/examples)を見ることである。
地道な方法はソースコードを見ることだが、`BuildSystem`があまりに複雑なので時間がかかるだろう。
個人的には[package](https://gitlab.com/petsc/petsc/-/tree/main/config/BuildSystem/config/packages)を見ると、ライブラリ毎のオプションがどういう風に使われているかわかるので、ここを見ることをお勧めする。
