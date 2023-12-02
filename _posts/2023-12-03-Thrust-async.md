# Thrustの非同期実行

初めまして。<br>
趣味で雪の物理シミュレーションをC++で書いています。<br>
アドベントカレンダーが空いていたので軽い気持ちで参加しました。<br>
よろしくお願いします。

Thrustの非同期実行についてですが、日本語の文献が<br>
[thrustにasyncサポートが入っていた](https://in-neuro.hatenablog.com/entry/2020/01/09/163007)<br>
くらいしか見当たらなかったので、具体例を交えながら環境構築から解説したいと思います。

## 環境構築
CMakeを使うやり方をご紹介します。<br>
Windowsでは試していないため、ご了承下さい。

以下のソフトウェアをインストールしてください。

- CUDA >= 12.3
- CMake >= 3.27.7
- gccまたはclang

Gitもインストールをお勧めしますが、なくても問題ありません。

### Thrustのダウンロード
[Thrust](https://github.com/NVIDIA/thrust/tree/main)をダウンロードします。<br>
1.9.4以上のバージョンを選んで下さい。<br>
Gitを使う方は再帰的にサブモジュールをクローンしてください。

なぜソースコードをダウンロードするかというと、CMakeがThrustを見つけてくれなかったためです。<br>
見つけ方をご存知の方は教えていただけると幸いです。

## CMakeプロジェクトの作成
例として、`float`と`int`の配列を作成し、GPUで各要素の2乗を計算し、結果が正しいか確認するプログラムを作成します。<br>
以下のようにCMakeプロジェクトを作成します。

```
hoge/-- CMakeLists.txt
     |- double.cu
     |- double.h
     |- main.cpp
     |- thrust/
```

`CMakeLists.txt`に以下を書きます。
```
cmake_minimum_required(VERSION 3.27.7)

# CUDAアーキテクチャを指定する。
# 最初に設定しないと、プロジェクトが作成できません。
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
  set(CMAKE_CUDA_ARCHITECTURES native)
endif()

project(my_program VERSION 0.1.0 LANGUAGES CXX CUDA)

# ここの設定はお好みで。
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OPTIMIZE_DEPENDENCIES ON)

# CUDAを見つける。
find_package(CUDAToolkit 12 REQUIRED)

# Thrustを見つける。
add_subdirectory(thrust)
find_package(Thrust REQUIRED CONFIG)
thrust_create_target(Thrust)

# CUDAライブラリを作る。
add_library(double double.cu)
# ラムダ式をデバイスコードで使えるようにし、SIMDを無効にする。
# SIMDの無効化は必要ありませんが、Eigenなど線形代数ライブラリを使う際に必要になります。
target_compile_options(
  double
  PRIVATE
    -expt-extended-lambda
    "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:-fno-tree-vectorize>"
    "$<$<COMPILE_LANG_AND_ID:CXX,Clang>:-fno-vectorize>"
)
# ここはお好みで。
target_compile_features(
  double
  PRIVATE
    cuda_std_20
)
# 誰もインストールしないはずなので、簡略に。
target_include_directories(
  double
  PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
# CUDAランタイムとThrustをリンクする。
# 当たり前ですが、ThrustにはCUDAはないので。
target_link_libraries(
  double
  PUBLIC
    CUDA::cudart
    Thrust
)

# my_programを作る。
add_executable(my_program main.cpp)
target_link_libraries(
  my_program
  PRIVATE
    double
)
# ここはお好みで。
target_compile_features(
  my_program
  PRIVATE
    cxx_std_20
)
# ここはお好みで。
target_compile_options(
  my_program
  PRIVATE
    "$<$<CONFIG:Debug>:-g;-O0;-Wall;-Wextra>"
    "$<$<CONFIG:Release>:-DNDEBUG;-O3;-march=native;-mtune=native>"
)
# CUDA 5.0以前のバージョンでは、デバイスコードで使用する関数の実装までインクルードする必要が
# ありましたが、CUDA 5.0以降では、デバイスコードで使用する関数の宣言のみをインクルードするだけで
# 十分になりました。
# このことをコンパイラとリンカーに伝えるために、CUDA_SEPARABLE_COMPILATIONプロパティをONに
# しています。
# ただし、このプロパティは実行ファイルにのみONに設定して下さい。
# 詳しくは参考文献1を参照してください。
set_property(TARGET my_program PROPERTY CUDA_SEPARABLE_COMPILATION ON)
```

## Thrustの非同期実行の書き方
いよいよ本題です。<br>

### eventとfuture
Thrustには`event`と`future`があります。<br>
`future`は`reduce`から返され、その他のアルゴリズムは`event`を返します。<br>
どちらも`wait()`メンバ関数を持ち、非同期実行が終わるまで待ちます。<br>
`future`はさらに`get()`メンバ関数を持ち、実行結果を取得できます。<br>
詳しく知りたい方は参考文献2のプログラムをご覧ください。<br>
`event`は`unique_eager_event`クラスに, `future`は`unique_eager_future`クラスに対応しています。<br>

ヘッダーはそれぞれ`thrust/event.h`, `thrust/future.h`です。<br>
今回は計算結果を`host_vector`に格納するので、使うのは実行が完了したか確認するだけの`event`です。

double.h
```
#include <thrust/event.h>
#include <thrust/host_vector.h>

thrust::device_event Double(thrust::host_vector<float>& doubles, thrust::host_vector<int>& ints);
```

ここで`device_event`を使っていますが、ホストでも使えます。<br>
`host_event`はありません。<br>
後の「欠点」の節でこのことに触れます。<br>
`__global__`はあってもなくてもいいです。


### 非同期なアルゴリズムと実行ポリシー
Thrustには`copy`や`transform`といった、STLのような関数が用意されています。<br>
これらを非同期に実行したい場合は、`async`名前空間のものを使えばいいです。<br>
例えば、`thrust::copy`の非同期版は`thrust::async::copy`です。

実行ポリシーとは、アルゴリズムにホストで実行するのか、デバイスで実行するのか、それとも逐次的に処理するのかを指定するクラスです。<br>
クラス名はそれぞれ`host`、`device`、`seq`です。<br>
アルゴリズムに明示的に指定することで、無駄なデータのコピーを減らせます。<br>
実装はタグディスパッチで、自分でカスタマイズすることも可能です。<br>
詳しくは参考文献3をご覧ください。

実行ポリシー自体は同期的なアルゴリズムに対しても使えますが、非同期なら`host`と`device`の`after`メンバ関数に`event`と`future`を指定することで実行順序を指定できます。<br>
`event`と`future`の個数は何個でもいいです。<br>
また、`thrust::when_all`関数で複数の`event`と`future`を一つの`event`にまとめることもできます。

これらを踏まえて、以下のように実装してみます。

double.cu
```
#include <thrust/async/copy.h>
#include <thrust/async/transform.h>
#include <thrust/device_vector.h>

#include "double.h"

thrust::device_event Double(thrust::host_vector<float>& floats, thrust::host_vector<int>& ints) {
  // デバイス側の配列を用意
  auto device_floats = thrust::device_vector<float>();
  auto device_ints   = thrust::device_vector<int>();

  // メモリの確保と初期化
  device_floats.resize(floats.size());
  device_ints.resize(ints.size());

  // ホストからデバイスへの非同期コピー
  auto copy_ints_event = thrust::async::copy(
      thrust::host,
      thrust::device,
      ints.begin(),
      ints.end(),
      device_ints.begin()
  );
  auto copy_floats_event = thrust::async::copy(
      thrust::host,
      thrust::device,
      floats.begin(),
      floats.end(),
      device_floats.begin()
  );

  // デバイス側での計算
  auto double_ints_event = thrust::async::transform(
      thrust::device.after(copy_ints_event),
      device_ints.begin(),
      device_ints.end(),
      device_ints.begin(),
      [] __device__(int i) { return i * i; }
  );
  auto double_floats_event = thrust::async::transform(
      thrust::device.after(copy_floats_event),
      device_floats.begin(),
      device_floats.end(),
      device_floats.begin(),
      [] __device__(float d) { return d * d; }
  );

  // デバイスからホストへの非同期コピー
  auto copy_back_ints_event = thrust::async::copy(
      thrust::device.after(double_ints_event),
      device_ints.begin(),
      device_ints.end(),
      ints.begin()
  );
  auto copy_back_floats_event = thrust::async::copy(
      thrust::device.after(double_floats_event),
      device_floats.begin(),
      device_floats.end(),
      floats.begin()
  );
  
  // 非同期実行の完了をまとめる
  return thrust::when_all(copy_back_ints_event, copy_back_floats_event);
}
```
`int`と`float`の配列のそれぞれに対して、デバイスへのコピーが完了したら、2乗する計算を行い、終わればホストへコピーする処理を順番に実行ポリシーに指定しています。<br>
最後に、両方の配列がホストへコピーされたことを確認するための`event`を作成しています。

あとは、`main`関数で各配列を作って、`Double`関数に渡して、結果が正しいか確認するだけです。

main.cpp
```
#include "double.h"

#include <cassert>
#include <cmath>
#include <iostream>

int main() {
  auto floats = thrust::host_vector<float>();
  auto ints   = thrust::host_vector<int>();

  std::size_t size = 10000;

  floats.reserve(size);
  ints.reserve(size);

  for (std::size_t i = 0; i < size; ++i) {
    floats.push_back(i);
    ints.push_back(i);
  }

  auto event = Double(floats, ints);

  event.wait();

  for (std::size_t i = 0; i < size; ++i) {
    assert(std::abs(floats[i] - i * i) < 1e-5);
    assert(ints[i] == static_cast<int>(i * i));
  }
  std::cout << "Success!\n";

  return 0;
}
```
本当は浮動小数点数の値の比較をもう少し丁寧にするべきですが、本題と関係ないので良しとします。

## 利点
一般に非同期実行というと身構える方が多いと思われますが、上の解説を見ると意外と簡単にできると思われたのではないでしょうか。<br>
また、実行ポリシーで実行順序を指定できるのも魅力的です。

非同期処理の利点というよりThrustを使う利点になりますが、実は`rocThrust`という、CUDAをHIPとROCmに置き換えたThrustをAMDが開発しています。<br>
現時点では、Thrust 1.17.2まで対応していますので非同期実行もできます。<br>
詳しくは参考文献4をご覧ください。

ちなみに、私はThrustのリポジトリのREADME.mdすらろくに見ずにリリースノートとソースコードから探してきたので、耐え難きを耐え、忍び難きを忍んだ気分でした(笑)。<br>

## 欠点
残念ながら欠点があります。<br>
1つ目の欠点はCUDAがデバイスの時かつデバイス側でしか使えないことです。<br>
ThrustはoneTBBをデバイスとして使うこともできますが、非同期処理はoneTBBでは実装されていません。<br>
シリアルや`OpenMP`も同様です。<br>
「eventとfuture」節で触れましたが、これが`host_event`がない理由だと考えられます。<br>
しかし、この欠点はさほど問題にならないでしょう。

2つ目の欠点は`async::copy`が転送するデータの型に対して[trivially relocatable](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r9.html)であることを要求していることです。<br>
`trivially relocatable`な型とは、簡単に言うとコピーとムーブ、破棄が`default`で出来て、仮想メンバ関数と仮想継承を持たず、全てのメンバ変数と基底クラスも`trivially relocatable`な型です。<br>
これは`memcpy`でホストとデバイス間でデータ転送できるようにするために導入されました。<br>
Thrustでは、`THRUST_PROCLAIM_TRIVIALLY_RELOCATABLE`マクロ関数を使って、与えた型が`trivially relocatable`であると宣言する必要があります。<br>
制約が強いもののパフォーマンスは向上し、[私が行った実験](https://github.com/AcademySoftwareFoundation/Imath/pull/337#issuecomment-1689218474)ではそうでないものに比べて実行時間が約30%減っています。<br>
詳しくは参考文献5をご覧ください。


## 実は...
Thrustのリポジトリをご覧になるとわかるのですが、なんとアーカイブにされてます。<br>
どうやら半年前から[CCCL](https://github.com/NVIDIA/cccl)という、CUBとlibcudacxxも一緒にしたリポジトリに移ったようです。<br>
時間の流れが速いと感じる、おじいちゃんの気分です(笑)。<br>
このことに気づいたのが一昨日だったので、昨日CCCLでもビルドできるか試していたのですが、上述のやり方ではエラーが出てしまいました。<br>
どうにか解決したかったのですが、時間もなかったのでとりあえずThrustに対して書きました。<br>
もしご存知の方がいらっしゃったら、教えていただけると幸いです。


## まとめ
`trivially relocatable`な型なら、お手軽に非同期実行できます。

## ソースコード
[GitHub](https://github.com/sukeya/ThrustAsyncProgram)に置いています。

## 参考文献
1. https://developer.nvidia.com/blog/separate-compilation-linking-cuda-device-code/
2. https://github.com/NVIDIA/thrust/blob/main/thrust/system/cuda/detail/future.inl
3. https://github.com/NVIDIA/thrust/blob/main/thrust/execution_policy.h
4. https://github.com/ROCmSoftwarePlatform/rocThrust
5. https://github.com/NVIDIA/thrust/releases/tag/1.9.4
