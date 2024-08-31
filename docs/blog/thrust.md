---
layout: post
title: Thrustの使い方
date: 2023-03-25 10:20:00 +0900
---
[Thrust](https://github.com/NVIDIA/thrust)を使う機会があったので、[ドキュメント](https://docs.nvidia.com/cuda/thrust/index.html)を簡単に翻訳する。

## 紹介
ThrustとはSTLに基づいた、CUDAのためのC++テンプレートライブラリである。
Thrustによって、CUDA Cと完全な互換性がある高水準のインターフェイスを通して、最小限の労力で高パフォーマンスな並列アプリケーションを実装することができる。

### インストール
CUDAツールキットをインストールすると、Thrustのヘッダファイルも標準のCUDAインクルードディレクトリにコピーされる。
Thrustはテンプレートライブラリなので、これ以上することはない。

## Vectors
Thrustは2つのvectorコンテナ、`host_vector`と`device_vector`を提供している。
名前が示すように、`device_vector`はGPUのメモリにある一方で、`host_vector`はホストメモリに保存される。
Thrustのvectorコンテナは`std::vector`に似ている。
`std::vector`のように、`host_vector`と`device_vector`は動的にサイズを変えることが出来る、ジェネリックコンテナ(任意のデータ型を持てる)である。
以下のソースコードはThrustのvectorコンテナの使い方を示す。

```
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>

#include <iostream>

int main(void)
{
    // H has storage for 4 integers
    thrust::host_vector<int> H(4);

    // initialize individual elements
    H[0] = 14;
    H[1] = 20;
    H[2] = 38;
    H[3] = 46;

    // H.size() returns the size of vector H
    std::cout << "H has size " << H.size() << std::endl;

    // print contents of H
    for(int i = 0; i < H.size(); i++)
        std::cout << "H[" << i << "] = " << H[i] << std::endl;

    // resize H
    H.resize(2);

    std::cout << "H now has size " << H.size() << std::endl;

    // Copy host_vector H to device_vector D
    thrust::device_vector<int> D = H;

    // elements of D can be modified
    D[0] = 99;
    D[1] = 88;

    // print contents of D
    for(int i = 0; i < D.size(); i++)
        std::cout << "D[" << i << "] = " << D[i] << std::endl;

    // H and D are automatically deleted when the function returns
    return 0;
}
```

この例が示すように、`=`演算子は`host_vector`から`device_vector`へコピーするために使われる(逆も同様)。
もちろん、`host_vector`同士や`device_vector`同士でコピーするためにも使われる。
また、`device_vector`の個々の要素は大かっこを使ってアクセスできるが、各アクセスは`cudaMemcpy`を呼び出す必要があるため、出来る限り使わない方が良い。

特定の値でvectorのすべての要素を初期化したり、あるvectorのある値の集合を別のvectorにコピーだけしたりすることが便利なことが多い。
Thrustはこれらの操作を行ういくつかの方法を提供している。

```
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>

#include <thrust/copy.h>
#include <thrust/fill.h>
#include <thrust/sequence.h>

#include <iostream>

int main(void)
{
    // initialize all ten integers of a device_vector to 1
    thrust::device_vector<int> D(10, 1);

    // set the first seven elements of a vector to 9
    thrust::fill(D.begin(), D.begin() + 7, 9);

    // initialize a host_vector with the first five elements of D
    thrust::host_vector<int> H(D.begin(), D.begin() + 5);

    // set the elements of H to 0, 1, 2, 3, ...
    thrust::sequence(H.begin(), H.end());

    // copy all of H back to the beginning of D
    thrust::copy(H.begin(), H.end(), D.begin());

    // print D
    for(int i = 0; i < D.size(); i++)
        std::cout << "D[" << i << "] = " << D[i] << std::endl;

    return 0;
}
```

`copy`関数はホストかデバイスの要素の範囲を別のホストかデバイスのvectorにコピーする。
`thrust::fill`関数は単に範囲内の要素に特定の値を入れる。
`sequence`関数は等間隔の値の列を作成するために使うことができる。

### 2.1 Thrustの名前空間
Thrustの名前空間は`thrust`。

### 2.2 イテレータと静的ディスパッチ
生のポインターをThrustの関数に引数として渡すこともできる。
ただし、デバイスメモリへのポインターは`thrust::device_ptr`で包まなければならない。
例えば、
```
size_t N = 10;

// raw pointer to device memory
int * raw_ptr;
cudaMalloc((void **) &raw_ptr, N * sizeof(int));

// wrap raw pointer with a device_ptr
thrust::device_ptr<int> dev_ptr(raw_ptr);

// use device_ptr in thrust algorithms
thrust::fill(dev_ptr, dev_ptr + N, (int) 0);
```

`device_ptr`から生のポインタを取り出すには、`raw_pointer_cast`を使うべきだ。
```
size_t N = 10;

// create a device_ptr
thrust::device_ptr<int> dev_ptr = thrust::device_malloc<int>(N);

// extract raw pointer from device_ptr
int * raw_ptr = thrust::raw_pointer_cast(dev_ptr);
```

イテレータとポインタを区別するもう一つの理由は、イテレータは多くの種類のデータ構造を横断するために使えるからである。
例えば、STLは双方向イテレータを提供するリンクリストを提供している。
Thrustはそのようなコンテナのデバイスの実装は提供していないが、それらと互換性がある。
```
#include <thrust/device_vector.h>
#include <thrust/copy.h>
#include <list>
#include <vector>

int main(void)
{
    // create an STL list with 4 values
    std::list<int> stl_list;

    stl_list.push_back(10);
    stl_list.push_back(20);
    stl_list.push_back(30);
    stl_list.push_back(40);

    // initialize a device_vector with the list
    thrust::device_vector<int> D(stl_list.begin(), stl_list.end());

    // copy a device_vector into an STL vector
    std::vector<int> stl_vector(D.size());
    thrust::copy(D.begin(), D.end(), stl_vector.begin());

    return 0;
}
```

Thrustは`counting_iterator`や`zip_iterator`といった、装飾的なイテレータの集まりも提供する。

## 3. アルゴリズム
Thrustはたくさんのありふれた並列アルゴリズムを提供している。
これらのアルゴリズムの多くはSTLの直接的な類似を持ち、同じSTL関数が存在する時は同じ名前を使う(例: `thrust::sort`と`std::sort`)。

Thrustの全てのアルゴリズムはホストとデバイスの両方に対する実装を持つ。
特に、Thrustのアルゴリズムがホストイテレータと一緒に呼び出された時、ホストパスがディスパッチされる。
同様に、デバイスイテレータが範囲を定義するために使われている時、デバイスの実装が呼ばれる。

ホストとデバイス間でデータをコピーできる`thrust::copy`という例外はあるが、Thrustアルゴリズムへの全てのイテレータの引数は同じ場所(全てホスト上か全てデバイス上)にあるべきだ。
この要件を破った時、エラーメッセージが作られる。

### 3.1 変形
完全なリストは[ここ](https://thrust.github.io/doc/group__transformations.html)。

以下のソースコードはいくつかの変形アルゴリズムを示している。
`thrust::negate`や`thrust::modulus`などのありふれたfunctorは`thrust/functional.h`に定義されている。

```
#include <thrust/device_vector.h>
#include <thrust/transform.h>
#include <thrust/sequence.h>
#include <thrust/copy.h>
#include <thrust/fill.h>
#include <thrust/replace.h>
#include <thrust/functional.h>
#include <iostream>

int main(void)
{
    // allocate three device_vectors with 10 elements
    thrust::device_vector<int> X(10);
    thrust::device_vector<int> Y(10);
    thrust::device_vector<int> Z(10);

    // initialize X to 0,1,2,3, ....
    thrust::sequence(X.begin(), X.end());

    // compute Y = -X
    thrust::transform(X.begin(), X.end(), Y.begin(), thrust::negate<int>());

    // fill Z with twos
    thrust::fill(Z.begin(), Z.end(), 2);

    // compute Y = X mod 2
    thrust::transform(X.begin(), X.end(), Z.begin(), Y.begin(), thrust::modulus<int>());

    // replace all the ones in Y with tens
    thrust::replace(Y.begin(), Y.end(), 1, 10);

    // print Y
    thrust::copy(Y.begin(), Y.end(), std::ostream_iterator<int>(std::cout, "\n"));

    return 0;
}
```

SAXPYの実装について書いてあるが、省略。

functorは以下のように作成できる。
```
struct saxpy_functor
{
    const float a;

    saxpy_functor(float _a) : a(_a) {}

    __host__ __device__
        float operator()(const float& x, const float& y) const {
            return a * x + y;
        }
};
```

複雑な計算は、簡単な計算に分解して何回かに分けて全体を計算するより、複雑な計算を1回でまとめて全体を計算するほうが早い。
例えば、配列`a`, `b`, `x`に対して、`a * x + b`を計算するなら、`a * x`を計算して`b`を足すより`a[i] * x[i] + b[i]`を計算するほうが早い。

`thrust::transform`は1つか2つの入力引数しか持たないが、[この例](https://github.com/NVIDIA/thrust/blob/master/examples/arbitrary_transformation.cu)を使うと2つより多い入力でも1回で出来る。

### 3.2 Reductions
特に気を付けることはない。
詳細は[ドキュメント](http://thrust.github.com/doc/group__reductions.html)を参照。

### 3.3 Prefix-Sums

