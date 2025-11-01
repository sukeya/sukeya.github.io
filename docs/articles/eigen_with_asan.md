# Address sanitizerとEigenを一緒に使うとき

`EIGEN_MAX_ALIGN_BYTES`マクロを`0`と定義すること。

## 理由
`Eigen`では、`EIGEN_MAX_ALIGN_BYTES`マクロと`EIGEN_MAX_STATIC_ALIGN_BYTES`マクロによって、それぞれ動的または静的に確保されたデータのメモリアラインメントの最大値を指定することができる。
`EIGEN_MAX_STATIC_ALIGN_BYTES <= EIGEN_MAX_ALIGN_BYTES`でなければならず、成り立たない場合は自動的に`EIGEN_MAX_STATIC_ALIGN_BYTES = EIGEN_MAX_ALIGN_BYTES`とされる。
もし、どちらも指定されなければアーキテクチャやコンパイラ、OSによって自動的に計算される。

`Eigen`はバイナリ互換性のためにベクトル化が無効化されていても強制的にメモリアラインメントを指定するため、Address sanitizerと一緒に使う時は`0`を指定してアラインメントを無効にする必要がある。

## 参考
- [Preprocessor directives](https://libeigen.gitlab.io/eigen/docs-5.0/TopicPreprocessorDirectives.html)
- [Eigen/src/Core/util/ConfigureVectorization.h](https://gitlab.com/libeigen/eigen/-/blob/master/Eigen/src/Core/util/ConfigureVectorization.h)
