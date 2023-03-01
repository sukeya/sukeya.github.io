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
- Alembic::Abc Alembicの基本的なインターフェイスを提供する。
- Alembic::AbcGeom Alembic::Abcを使って、特定の幾何学の物体(三角形とか)を実装する。

各ライブラリにはそのライブラリが公開している全てをインクルードした、`All.h`という名前のヘッダーがある。
なので、以下のようにインクルードすればよい。
```
// Alembic Includes
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
```

各ライブラリは自身と同じ名前の名前空間を持つ。
簡潔さのために省略する。
```
using namespace Alembic::AbcGeom; // Abc、AbcCoreAbstractを含む
```

次に`Archive`を作り、その`Archive`の子供として、静的な多角形メッシュを持つアニメーションを追加しよう。

```
// OArchiveを作る。
// std::iostreamsと同様に、(OArchive、IArchiveなど)入出力に対して、完全に分離され並列化可能なクラス階層を持つ。
// これによって、動的なシーン操作フレームワークとは反対に、Alembicはストレージ、表現、アーカイブ化に対して重要な抽象化を保っている。
OArchive archive(
    // Archiveの書き込みを実装しているクラスのインスタンスを渡す。
    Alembic::AbcCoreOgawa::WriteArchive(),

    // ファイル名
    // OArchiveなので、このファイル名を持つアーカイブを作成する。
    "polyMesh1.abc"
);
```


```
    // Create a PolyMesh class.
    OPolyMesh meshyObj( OObject( archive, kTop ), "meshy" );
    OPolyMeshSchema &mesh = meshyObj.getSchema();

    // some apps can arbitrarily name their primary UVs, this function allows
    // you to do that, and must be done before the first time you set UVs
    // on the schema
    mesh.setUVSourceName("test");

    // UVs and Normals use GeomParams, which can be written or read
    // as indexed or not, as you'd like.
    OV2fGeomParam::Sample uvsamp( V2fArraySample( (const V2f *)g_uvs,
                                                  g_numUVs ),
                                  kFacevaryingScope );
    // indexed normals
    ON3fGeomParam::Sample nsamp( N3fArraySample( (const N3f *)g_normals,
                                                 g_numNormals ),
                                 kFacevaryingScope );

    // Set a mesh sample.
    // We're creating the sample inline here,
    // but we could create a static sample and leave it around,
    // only modifying the parts that have changed.
    OPolyMeshSchema::Sample mesh_samp(
        V3fArraySample( ( const V3f * )g_verts, g_numVerts ),
        Int32ArraySample( g_indices, g_numIndices ),
        Int32ArraySample( g_counts, g_numCounts ),
        uvsamp, nsamp );

    // not actually the right data; just making it up
    Box3d cbox;
    cbox.extendBy( V3d( 1.0, -1.0, 0.0 ) );
    cbox.extendBy( V3d( -1.0, 1.0, 3.0 ) );

    // Set the sample twice
    mesh.set( mesh_samp );
    mesh.set( mesh_samp );

    // do it twice to make sure getChildBoundsProperty works correctly
    mesh.getChildBoundsProperty().set( cbox );
    mesh.getChildBoundsProperty().set( cbox );

    // Alembic objects close themselves automatically when they go out
    // of scope. So - we don't have to do anything to finish
    // them off!
    std::cout << "Writing: " << archive.getName() << std::endl;
}
```

# 参考文献
1. [Introduction &mdash; Alembic 1.7.0 documentation](http://docs.alembic.io/python/examples.html#properties)
2. [alembic/lib/Alembic/AbcGeom/Tests/PolyMeshTest.cpp](https://github.com/alembic/alembic/blob/master/lib/Alembic/AbcGeom/Tests/PolyMeshTest.cpp)
