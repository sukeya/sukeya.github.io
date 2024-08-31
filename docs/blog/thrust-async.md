# Thrustの非同期実行

Thrustの非同期実行についてですが、日本語の文献が『[thrustにasyncサポートが入っていた](https://in-neuro.hatenablog.com/entry/2020/01/09/163007)』くらいしか見当たらなかったので、具体例を交えながら環境構築から解説したいと思います。


## 環境構築
CMakeを使うやり方をご紹介します。
Windowsでは試していないため、ご了承下さい。

以下のソフトウェアをインストールしてください。

- CUDA >= 12.3
- CMake >= 3.27.7
- GCC >= 5.1 または Clang >= 3.4

Gitもインストールをお勧めしますが、なくても問題ありません。


### Thrustのダウンロード
[Thrust](https://github.com/NVIDIA/thrust/tree/main)をダウンロードします。
1.9.4以上のバージョンを選んで下さい。
Gitを使う方は再帰的にサブモジュールをクローンしてください。

なぜソースコードをダウンロードするかというと、CMakeがThrustを見つけてくれなかったためです。
見つけ方をご存じの方は教えていただけると幸いです。


## CMakeプロジェクトの作成
例として、`float`と`int`の配列を作成し、GPUで各要素の2乗を計算し、結果が正しいか確認するプログラムを作成します。
以下のようにCMakeプロジェクトを作成します。

```
hoge/-- CMakeLists.txt
     |- double.cu
     |- double.h
     |- main.cpp
     |- thrust/
```

CMakeLists.txtに以下を書きます。

```cmake
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
target_compile_options(
  double
  PRIVATE
    # C++14以上が必要
    cxx_std_14
    # ラムダ式をデバイスコードで使えるようにする。
    -expt-extended-lambda
    # SIMDを無効にする。
    # SIMDの無効化は必須ではありませんが、Eigenなど線形代数ライブラリを使う際に必要になります。
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
いよいよ本題です。


### eventとfuture
Thrustには`event`と`future`があります。
`future`は`reduce`から返され、その他のアルゴリズムは`event`を返します。
どちらも`wait()`メンバ関数を持ち、非同期実行が終わるまで待ちます。
`future`はさらに`get()`メンバ関数を持ち、実行結果を取得できます。
詳しく知りたい方は参考文献2のプログラムをご覧ください。
`event`は`unique_eager_event`クラスに, `future`は`unique_eager_future`クラスに対応しています。

ヘッダーはそれぞれthrust/event.h, thrust/future.hです。
今回は計算結果を`host_vector`に格納するので、使うのは実行が完了したか確認するだけの`event`です。

double.h

```cpp
#include <thrust/event.h>
#include <thrust/host_vector.h>

thrust::device_event Double(thrust::host_vector<float>& doubles, thrust::host_vector<int>& ints);
```

ここで`device_event`を使っていますが、ホストでも使えます。
`host_event`はありません。
後の「欠点」の節でこのことに触れます。
`__global__`はあってもなくてもいいです。


### 非同期なアルゴリズムと実行ポリシー
Thrustには`copy`や`transform`といった、STLのような関数が用意されています。
これらを非同期に実行したい場合は、`async`名前空間のものを使えばいいです。
例えば、`thrust::copy`の非同期版は`thrust::async::copy`です。

実行ポリシーとは、アルゴリズムにホストで実行するのか、デバイスで実行するのか、それとも逐次的に処理するのかを指定するクラスです。
クラス名はそれぞれ`host`、`device`、`seq`です。
アルゴリズムに明示的に指定することで、無駄なデータのコピーを減らせます。
実装はタグディスパッチで、自分でカスタマイズすることも可能です。
詳しくは参考文献3をご覧ください。

実行ポリシー自体は同期的なアルゴリズムに対しても使えますが、非同期なら`host`と`device`の`after`メンバ関数に`event`と`future`を指定することで実行順序を指定できます。
`event`と`future`の個数は何個でもいいです。
また、`thrust::when_all`関数で複数の`event`と`future`を一つの`event`にまとめることもできます。

これらを踏まえて、以下のように実装してみます。

double.cu

```cuda
#include <thrust/async/copy.h>
#include <thrust/async/transform.h>
#include <thrust/device_vector.h>

#include "double.h"

template <class T>
thrust::device_event Double(thrust::host_vector<T>& ts) {
  // デバイス側の配列を用意
  auto device_ts = thrust::device_vector<T>();

  // メモリの確保と初期化
  device_ts.resize(ts.size());

  // ホストからデバイスへの非同期コピー
  auto copy_ts_event = thrust::async::copy(
      thrust::host,
      thrust::device,
      ts.begin(),
      ts.end(),
      device_ts.begin()
  );

  // デバイス側での計算
  auto double_ts_event = thrust::async::transform(
      thrust::device.after(copy_ts_event),
      device_ts.begin(),
      device_ts.end(),
      device_ts.begin(),
      [] __device__(T d) { return d * d; }
  );

  // デバイスからホストへの非同期コピー
  auto copy_back_ts_event = thrust::async::copy(
      thrust::device.after(double_ts_event),
      device_ts.begin(),
      device_ts.end(),
      ts.begin()
  );
  
  return copy_back_ts_event;
}

thrust::device_event Double(thrust::host_vector<float>& floats, thrust::host_vector<int>& ints) {
  // 非同期実行の完了をまとめる
  return thrust::when_all(Double(floats), Double(ints));
}
```

`int`と`float`の配列のそれぞれに対して、デバイスへのコピーが完了したら、2乗する計算を行い、終わればホストへコピーする処理を順番に実行ポリシーに指定しています。
最後に、両方の配列がホストへコピーされたことを確認するための`event`を作成しています。

あとは、`main`関数で各配列を作って、`Double`関数に渡して、結果が正しいか確認するだけです。

main.cpp

```cpp
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
一般に非同期実行というと身構える方が多いと思われますが、上の解説を見ると意外と簡単にできると思われたのではないでしょうか。
また、実行ポリシーで実行順序を指定できるのも魅力的です。

非同期処理の利点というよりThrustを使う利点になりますが、実はrocThrustという、CUDAをHIPとROCmに置き換えたThrustをAMDが開発しています。
現時点では、Thrust 1.17.2まで対応していますので非同期実行もできます。
詳しくは参考文献4をご覧ください。


## 欠点
残念ながら欠点があります。
1つ目の欠点はCUDAがデバイスの時かつデバイス側でしか使えないことです。
ThrustはoneTBBをデバイスとして使うこともできますが、非同期処理はoneTBBでは実装されていません。
シリアルやOpenMPも同様です。
「eventとfuture」節で触れましたが、これが`host_event`がない理由だと考えられます。
しかし、この欠点はさほど問題にならないでしょう。

2つ目の欠点は`async::copy`が転送するデータの型に対して[trivially relocatable](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r9.html)であることを要求していることです。
trivially relocatableな型とは、簡単に言うとコピーとムーブ、破棄が`default`で出来て、仮想メンバ関数と仮想継承を持たず、全てのメンバ変数と基底クラスもtrivially relocatableな型です。
これは`memcpy`でホストとデバイス間でデータ転送できるようにするために導入されました。
Thrustでは、`THRUST_PROCLAIM_TRIVIALLY_RELOCATABLE`マクロ関数を使って、与えた型がtrivially relocatableであると宣言する必要があります。
制約が強いもののパフォーマンスは向上し、[私が行った実験](https://github.com/AcademySoftwareFoundation/Imath/pull/337#issuecomment-1689218474)ではそうでないものに比べて実行時間が約30%減っています。
詳しくは参考文献5をご覧ください。


## 実は...
Thrustのリポジトリをご覧になるとわかるのですが、なんとアーカイブにされてます。
どうやら半年前から[CCCL](https://github.com/NVIDIA/cccl)という、CUBとlibcudacxxも一緒にしたリポジトリに移ったようです。
このことに気づいたのが一昨日だったので、昨日CCCLでもビルドできるか試していたのですが、上述のやり方ではエラーが出てしまいました。
どうにか解決したかったのですが、時間もなかったのでとりあえずThrustに対して書きました。
もしご存じの方がいらっしゃったら、教えていただけると幸いです。


## まとめ
データがtrivially relocatableな型なら、お手軽に非同期実行できます。


## ソースコード
[GitHub](https://github.com/sukeya/ThrustAsyncProgram)に置いています。


## 参考文献
1. https://developer.nvidia.com/blog/separate-compilation-linking-cuda-device-code/
2. https://github.com/NVIDIA/thrust/blob/main/thrust/system/cuda/detail/future.inl
3. https://github.com/NVIDIA/thrust/blob/main/thrust/execution_policy.h
4. https://github.com/ROCmSoftwarePlatform/rocThrust
5. https://github.com/NVIDIA/thrust/releases/tag/1.9.4
