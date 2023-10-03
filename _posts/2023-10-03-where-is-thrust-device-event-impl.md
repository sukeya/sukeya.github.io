---
layout: post
title:  thrust::device_eventの実装場所
date:   2023-10-02 11:10:00 +0900
---
`thrust`リポジトリで話をする。

`thrust::device_event`は`thrust/thrust/system/cuda/detail/future.inl`の`unique_eager_event`クラスのエイリアスなので、このクラスを見れば良い。
