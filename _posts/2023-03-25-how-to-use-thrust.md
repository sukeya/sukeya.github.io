---
layout: post
title: Thrustの使い方
date: 2023-03-25 10:20:00 +0900
---
[Thrust](https://github.com/NVIDIA/thrust)を使う機会があったので、使い方を簡単にまとめる。

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

