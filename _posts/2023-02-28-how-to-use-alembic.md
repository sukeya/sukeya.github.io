---
layout: post
title:  Alembicの使い方
date:   2023-03-02 17:15:00 +0900
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
Objectは直接データを持たないが、より直接的にデータを持つ構造体のための構造を提供する。
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

# 使用例
## ポリゴンメッシュの書き込み
以下の2つのライブラリを使う。
|ライブラリ名|機能|
|--|--|
|Alembic::Abc|Alembicの基本的なインターフェイスを提供する。|
|Alembic::AbcGeom|Alembic::Abcを使って、特定の幾何学の物体(三角形とか)を実装する。|

各ライブラリにはそのライブラリが公開している全てをインクルードした、`All.h`という名前のヘッダーがある。
なので、以下のようにインクルードすればよい。
```
// Alembic Includes
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
```

各ライブラリは自身と同じ名前の名前空間を持つ。
例えば、AbcGeomなら`Alembic::AbcGeom`である。

次に`Archive`を作り、その`Archive`の子供として、静的なポリゴンメッシュを持つアニメーションを追加しよう。
```
auto ostream = std::ofstream("polyMesh1.abc");
if (!ostream)
{
  throw std::runtime_error("Cannot open polyMesh1.abc.");
}

// メタデータの作成
auto abc_metadata = Alembic::Abc::MetaData();
// 名前を"PolyMesh"にする。
abc_metadata.set(Alembic::Abc::kUserDescriptionKey, "PolyMesh");

auto archive_writer = Alembic::AbcCoreOgawa::WriteArchive();
// ポインタを渡していることに注意！
auto writer_ptr = archive_writer(&ostream, abc_metadata);
// polyMesh1.abcへ書き込めるarchiveを作成
auto archive = Alembic::Abc::OArchive(writer_ptr);
```

立方体\[-1, 1\] x \[-1, 1\] x \[-1, 1\]のPolyMesh Objectを作成する。"meshy"は`meshyObj`の名前。
```
auto abc_top = archive.getTop();
auto meshyObj = OPolyMesh(abc_top, "meshy");
```

UVと法線のSampleを作成する。
その前に、UVの名前はSampleに入れる前に付ける必要がある。
```
auto& mesh = meshyObj.getSchema();
mesh.setUVSourceName("test");
```

UVと法線は`GeomParams`を使う。
`GeomParams`はインデックスがあってもなくても読み書きできる。
`kFacevaryingScope`については、[参考文献](#%E5%8F%82%E8%80%83%E6%96%87%E7%8C%AE)の3、4を参考。
```
// 立方体の頂点のUV。
// AbcGeomのPolyMeshクラスのUVはインデックスを持たない頂点毎、面毎である。
extern const size_t g_numUVs;
extern const Abc::float32_t g_uvs[];
auto uvsamp = OV2fGeomParam::Sample(
    V2fArraySample((const V2f *)g_uvs, g_numUVs), // reinterpret_castを使うべき
    kFacevaryingScope
);
// 立方体の頂点の法線
// AbcGeomのPolyMeshクラスの法線はインデックスを持たない頂点毎、面毎である。
// これは基本的にはRenderManの"facevarying"の型に合うストレージである。
extern const size_t g_numNormals;
extern const Abc::float32_t g_normals[];
auto nsamp = ON3fGeomParam::Sample(
    N3fArraySample((const N3f *)g_normals, g_numNormals),
    kFacevaryingScope
);
```

メッシュのSampleを設定する。
```
auto mesh_samp = OPolyMeshSchema::Sample(
    V3fArraySample((const V3f *)g_verts, g_numVerts),
    Int32ArraySample(g_indices, g_numIndices),
    Int32ArraySample(g_counts, g_numCounts),
    uvsamp,
    nsamp
);
mesh.set(mesh_samp);
```

Alembicのオブジェクトはスコープから出るときに自動的に破棄される。
なので、特に何もしなくて良い！

プログラム全体は参考文献の2にある。

# 参考文献
1. [Introduction &mdash; Alembic 1.7.0 documentation](http://docs.alembic.io/python/examples.html#properties)
2. [alembic/lib/Alembic/AbcGeom/Tests/PolyMeshTest.cpp](https://github.com/alembic/alembic/blob/master/lib/Alembic/AbcGeom/Tests/PolyMeshTest.cpp)
3. [alembic/lib/Alembic/AbcGeom/GeometryScope.h](https://github.com/alembic/alembic/blob/master/lib/Alembic/AbcGeom/GeometryScope.h)
4. [Geometric Primitives](https://renderman.pixar.com/resources/RenderMan_20/geometricPrimitives.html)
