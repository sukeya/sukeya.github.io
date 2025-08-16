# Ubuntu 25.04上のRadeonでRusticlを使う方法

RusticlはRustで実装されたオープンソースのOpenCL実装。
OpenCL 3.0を実装していて、2025年8月現在だと活発に開発されている。

1. MesaのOpenCL実装をインストール。ついでに`clinfo`も。
```sh
sudo apt install mesa-opencl-icd clinfo
```
2. インストールが正常に終わると、`clinfo`コマンドでOpenCL実装のリストとハードウェアのリストを見れるはず。
3. `~/.bashrc`に以下を書き込む[^1]
```sh
# RusticlのOpenCL実装をすべてのRadeonデバイスに対して有効化
export RUSTICL_ENABLE=radeonsi
```

3を行わないとCloverという古いOpenCL実装が読み込まれてしまう。

[^1]: https://docs.mesa3d.org/envvars.html#envvar-RUSTICL_ENABLE
