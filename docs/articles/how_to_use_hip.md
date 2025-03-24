# HIPの使い方

## HIPって何？
HIP (Heterogeneous-computing Interface for Portability) とは、一つのソースコードからCPUやAMDのGPU, NVIDIAのGPUを使ったアプリケーションを書けるC++ runtime APIとカーネル言語である。
HIPは、

- 直接CUDAやROCmで書いたコードとほぼ同じ性能を持ち、
- C++17以前のすべての言語機能をサポートし、C++20のほとんどの言語機能をサポートしていて、
- 特定のプラットフォーム(CUDAやROCm)に特化することもできる。

CUDAの関数をHIPの関数に変換する[`HIPify`](https://rocm.docs.amd.com/projects/HIPIFY/en/latest/index.html)という変換ツールもある。

## システム要件
### AMD
- [Linux](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/reference/system-requirements.html)
- [Windows](https://rocm.docs.amd.com/projects/install-on-windows/en/latest/reference/system-requirements.html)

### NVIDIA
Compute Capability 5.0以上のCUGAが使えるGPU。
詳細は[ここ](https://developer.nvidia.com/cuda-gpus)。

## インストール
### AMD
ROCmをインストールすると自動的にインストールされている。
まだインストールしていないなら、以下を参照:

- [Linux](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/index.html)
- [Windows](https://rocm.docs.amd.com/projects/install-on-windows/en/latest/index.html)

### NVIDIA
#### Linux
以下、Ubuntuを例にコマンドを紹介する[^1]。

1. [CUDA Toolkit](https://developer.nvidia.com/cuda-downloads)をインストールする。
2. radeonリポジトリのセットアップ
    1. [公式サイト](https://repo.radeon.com/amdgpu-install/)からパッケージをダウンロード。
    2. `sudo apt install ./amdgpu-install_(version)_all.deb`
    3. `sudo apt update`
3. `hip-runtime-nvidia`と`hip-dev`をインストール。
```bash
apt-get install hip-runtime-nvidia hip-dev
```
4. `HIP_PLATFORM`を`nvidia`に設定。
```bash
export HIP_PLATFORM="nvidia"
```

#### Windows
[ここ](https://rocm.docs.amd.com/projects/install-on-windows/en/latest/index.html)を参照。

[^1]: [hip/docs/install/install.rst](https://github.com/ROCm/hip/blob/amd-staging/docs/install/install.rst)
