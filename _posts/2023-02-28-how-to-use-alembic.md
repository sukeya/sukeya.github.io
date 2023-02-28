---
layout: post
title:  Alembicの使い方
date:   2023-02-28 22:48:00 +0900
---

# Alembicとは
Alembicはレンダリングや物理シミュレーションなどのソフトウェア間で共有できる、
時間ごとの物体の幾何情報を保存したオープンソースのファイル形式である。

# 主な概念
## Archive
ディスク上の実際のファイル。
すべてのシーンデータを持つトップレベルのコンテナ。
複数のObjectを持つ。

## Object
Alembicの階層化の主な単位。
例えば、Archiveをファイルシステム(例えば、ex4)とするなら、Objectはディレクトリである。
Objectは直接データを持たないが、代わりにより直接的にデータを持つ構造体のための構造を提供する。
複数のプロパティを持つ。
Archive直下のObject(TopObject)以外の全てのObjectは他のObjectの子供である。

### Schema

## Property
複数のSampleを持つ。
SimpleとCompoundの2種類の型がある。

### Simple Property
ScalarとArrayの2種類ある。

#### Scalar Property
型と要素の数が固定され、書く前にわかる(?)複数のSampleを持つ。
例として、以下がある。
- FloatProperty (各Sampleは32ビットの浮動小数点数; 長さ 1)
- StringProperty (各sampleは任意の長さを持つ、1つのstring; 長さ 1)
- M44dProperty (各Sampleは16個の64ビットの浮動小数点数; 長さ 16).
- 剛体の質量(1つの数値; 長さ 1)、
色(値が浮動小数点数のRGBのベクトル;長さ 3),
バウンディングボックス(値が浮動小数点数の2つの3次元ベクトル; 長さ 6)

ScalarPropertyの最大の長さは256である。

#### Array Properties
型が固定され、書く前にわかる(?)複数のSampleを持つが、長さが可変のSimple Propertyである。
例として、以下がある。
-  DoubleArrayProperty (each Sample is an array of varying length, each array element being a single 64-bit floating point number)
-  V3fArrayProperty (each Sample is an array of varying length, and each element in the array being an Imath::Vec3f, which is three 32-bit floating point numbers)
-  M44fArrayProperty (each Sample is an array of varying length, and each array element is an Imath::M44f, or sixteen 32-bit floating point numbers)
- 多角形メッシュの頂点のリスト
- 流体シミュレーションの粒子のリスト
ただし、同じSampleを保存しない。

### Compound Property
複数のPropertyを持つ。

## Sample
実際にデータを持っている構造体。

# 参考文献
1. [Introduction &mdash; Alembic 1.7.0 documentation](http://docs.alembic.io/python/examples.html#properties)
