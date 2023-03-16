---
layout: post
title:  Alembicの使い方
date:   2023-03-02 17:39:00 +0900
---
この記事はAlembicのv1.8.3を参照している。
意味がわからなくても最後まで目を通してから、もう一度読むことをお勧めする。
私が理解している`PolyMesh`(ポリゴンメッシュ)と`Xform`(メッシュの変形)に関連する項目のみ取り上げる。

## Alembicとは
Alembicはレンダリングや物理シミュレーションなどを行うCGソフトウェア間で共有できる、
時間ごとの物体の幾何情報を保存したオープンソースのファイル形式である。

## 主な概念
### Archive
ディスク上の実際のファイル。
すべてのシーンデータを持つトップレベルのコンテナ。
1つの`Object`を持つ。

### Object
Alembicの階層の主な単位。
`Object`は`Archive`の`Object`をルートとした多分木になっており、親と子供たちを取得することが出来る。
1つの`CompoundProperty`を持つ。
`ObjectHeader`と呼ばれるメタデータを持ち、`SchemaObject`を判定するために使われる。

### Schema
ある複雑なオブジェクト(例: ポリゴンメッシュ)を実装するために作られた`CompoundProperty`。

#### SchemaObject
`Schema`を`CompundProperty`として持つ`Object`のこと。

### Property
SimpleとCompoundの2種類の型がある。

#### Simple Property
ScalarとArrayの2種類ある。

##### Scalar Property
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

##### TypedArrayProperty
時刻毎の`Sample`を持つ`Property`。
`ArrayProperty`もあるが、主に使うのはこっちの方。
テンプレートパラメータに値の型(正確には[TypedPropertyTraits.h](https://github.com/alembic/alembic/blob/master/lib/Alembic/Abc/TypedPropertyTraits.h)で定義された型)が入っている。
例として、以下がある。
-  `DoubleArrayProperty` (各時刻で、要素が1つの64ビットの浮動小数点数の配列を持つ)
-  `V3fArrayProperty` ((各時刻で、要素が1つの`Imath::Vec3f`(3つの32ビットの浮動小数点数)の配列を持つ)
-  `M44fArrayProperty` (各時刻で、要素が1つのImath::M44f(16個の32ビットの浮動小数点数)の配列を持つ)
- 多角形メッシュの頂点のリスト
- 流体シミュレーションの粒子のリスト

ただし、同じSampleを保存しない。

値を取得するには`SampleSelector`という、ある時点の時刻を表すクラスを与える必要がある。

#### CompoundProperty
(Compound Propertyを含む)複数のPropertyを持つProperty。

### Sample
ある時刻の生のデータ。
Propertyと同様に、SampleもScalarとArrayの2種類ある。
例として、以下がある。
- `DoubleArraySample` (要素が1つの64ビットの浮動小数点数の配列)
- `V3fArraySample` (要素が1つの`Imath::Vec3f`(3つの32ビットの浮動小数点数)の配列)
- `M44fArraySample` (要素が1つの`Imath::M44f`(16個の32ビットの浮動小数点数)の配列)

### Time Sampling
Alembicがサポートしている時間のサンプリングは4種類ある。

#### Uniform
構築時に定義された時間刻み毎にサンプリングする。

#### Identity
デフォルト。各サンプルのインデックスがサンプリングの時刻と同じ。

#### Cyclic
時間刻み幅毎にある有限個だけサンプリングする: 例えば、シャッターの開閉。

#### Acyclic
時間刻み幅が任意で、どんなCyclicにも従わない。

## 使用例
### ポリゴンメッシュの書き込み
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
// メタデータの作成
auto abc_metadata = Alembic::Abc::MetaData();
// 名前を"PolyMesh"にする。
abc_metadata.set(Alembic::Abc::kUserDescriptionKey, "PolyMesh");

auto archive_writer = Alembic::AbcCoreOgawa::WriteArchive();
auto writer_ptr = archive_writer("polyMesh1.abc", abc_metadata);
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
// UVの値は指定した面を構成する頂点の順に、各面の各頂点の値を列挙する。
extern const size_t g_numUVs;
extern const Abc::float32_t g_uvs[];
auto uvsamp = OV2fGeomParam::Sample(
    V2fArraySample((const V2f *)g_uvs, g_numUVs), // reinterpret_castを使うべき
    kFacevaryingScope
);
// 立方体の頂点の法線
// 法線の値は指定した面を構成する頂点の順に、各面の各頂点の値を列挙する。
// これは基本的にはRenderManの"facevarying"の型に合うストレージである。
extern const size_t g_numNormals;
extern const Abc::float32_t g_normals[];
auto nsamp = ON3fGeomParam::Sample(
    N3fArraySample((const N3f *)g_normals, g_numNormals),
    kFacevaryingScope
);
```

メッシュのSampleを設定する。
引数は先頭から順に、頂点の座標と頂点の数、面を構成する頂点のインデックスのリストとそのリストのサイズ、面を構成する頂点の数のリストとそのリストのサイズである。
速度は`OPolyMeshSchema::Sample`の`setVelocityies`メンバ関数で設定する。
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

### ポリゴンメッシュの読み込み
インクルードするヘッダーと名前空間は前例を参照。

読み込み用アーカイブを作成する。
```
auto archive_reader = Alembic::AbcCoreOgawa::ReadArchive();
auto reader_ptr = archive_reader("polyMesh1.abc");
auto archive = Alembic::Abc::IArchive(reader_ptr);
```

UV, 法線を取得する。
```
auto meshyObj = IPolyMesh(archive.getTop(), "meshy");
IPolyMeshSchema &mesh = meshyObj.getSchema();
IN3fGeomParam N = mesh.getNormalsParam();
IV2fGeomParam uv = mesh.getUVsParam();
```

頂点の座標のリスト、面を構成する頂点のインデックスのリスト、面を構成する頂点の数のリスト、AABB、速度のリスト(もしあれば)を取得する。
```
IPolyMeshSchema::Sample mesh_samp;
mesh.get(mesh_samp);
```

以下のコードで、法線のリストのポインター(正確には`N3fArraySamplePtr`)を取得できる。
```
auto nsp = N.getExpandedValue().getVals();
```

各法線は以下のようにして取得できる。
```
for ( size_t i = 0 ; i < nsp->size() ; ++i )
{
    std::cout << i << "th normal: " << (*nsp)[i] << std::endl;
}
```

インデックスを付けたいときは、`getExpandedValue`の代わりに`getIndexedValue`を使う。
```
auto uvsamp = uv.getIndexedValue();
```

値は以下のように取得する。
```
V2f uv2 = (*(uvsamp.getVals()))[2];
TESTING_ASSERT( uv2 == V2f( 1.0f, 1.0f ) );
```

頂点のリストのサイズは以下で取得できる。
```
mesh_samp.getPositions()->size();
```

最初の頂点の座標は以下で取得できる。
```
(*(mesh_samp.getPositions()))[0];
```

取得するだけなら、以下のように簡潔に書ける。
```
mesh_samp.getPositions()->get()[0];
```

## 参考文献
1. [Introduction &mdash; Alembic 1.7.0 documentation](http://docs.alembic.io/python/examples.html#properties)
2. [alembic/lib/Alembic/AbcGeom/Tests/PolyMeshTest.cpp](https://github.com/alembic/alembic/blob/master/lib/Alembic/AbcGeom/Tests/PolyMeshTest.cpp)
3. [alembic/lib/Alembic/AbcGeom/GeometryScope.h](https://github.com/alembic/alembic/blob/master/lib/Alembic/AbcGeom/GeometryScope.h)
4. [Geometric Primitives](https://renderman.pixar.com/resources/RenderMan_20/geometricPrimitives.html)
