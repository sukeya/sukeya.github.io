---
layout: post
title:  Alembicの使い方
date:   2023-02-28 12:00:00 +0900
---
# Alembicとは
Alembicはレンダリングや物理シミュレーションなどのCGソフトウェア間で共有できる、
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
複数のPropertyを持つ。
Archive直下のObject(TopObject)以外の全てのObjectは他のObjectの子供である。

### Schema
ある複雑なObjectを実装するよう期待されたPropertyの最小限の集まり。
SchemaはCompound Propertyである。

## Property
複数のSampleを持つ。
SimpleとCompoundの2種類の型がある。

### Simple Property
ScalarとArrayの2種類ある。

#### Scalar Property
型と要素の数が固定され、書く前にわかる(?)複数のSampleを持つ。
例として、以下がある。
- FloatProperty (各Sampleは32ビットの浮動小数点数; 長さ 1)
- StringProperty (各Sampleは任意の長さを持つ、1つのstring; 長さ 1)
- M44dProperty (各Sampleは16個の64ビットの浮動小数点数; 長さ 16)
- 以下の3つのSampleを持つProperty
  - 剛体の質量(1つの数値; 長さ 1)
  - 色(値が浮動小数点数のRGBのベクトル;長さ 3)
  - バウンディングボックス(値が浮動小数点数の2つの3次元ベクトル; 長さ 6)

ScalarPropertyの最大の長さは256である。

#### Array Properties
型が固定され、書く前にわかる(?)複数のSampleを持つが、長さが可変のSimple Propertyである。
例として、以下がある。
-  DoubleArrayProperty (各Sampleは可変長配列で, 各配列要素は1つの64ビットの浮動小数点数)
-  V3fArrayProperty (各Sampleは可変長配列で, 各配列要素は1つのImath::Vec3f(3つの32ビットの浮動小数点数))
-  M44fArrayProperty (各Sampleは可変長配列で, 各配列要素は1つのImath::M44f(16個の32ビットの浮動小数点数))
- 多角形メッシュの頂点のリスト
- 流体シミュレーションの粒子のリスト

ただし、同じSampleを保存しない。

### Compound Property
(Compound Propertyを含む)複数のPropertyを持つ特別なProperty(というよりコンテナに近い)。

## Sample
生のデータとある時刻を一つに集約したコンテナである。
Propertyと同様に、SampleもScalarとArrayの2種類ある。

## Time Sampling
Alembicファイルは異なる時間のPropertyのSampleの列からなる。
Alembicがサポートしている時間のサンプリングは4種類ある。

### Uniform
構築時に定義された時間刻み毎にサンプリングする。

### Identity
デフォルト。各サンプルのインデックスがサンプリングの時刻と同じ。

### Cyclic
時間刻み幅毎にある有限個だけサンプリングする: 例えば、シャッターの開閉。

### Acyclic
時間刻み幅が任意で、どんなCyclicにも従わない。

# 使い方




# 参考文献
1. [Introduction &mdash; Alembic 1.7.0 documentation](http://docs.alembic.io/python/examples.html#properties)
