# Alembicの使い方
この記事はAlembicのv1.8.3を参照しています。
意味がわからなくても最後まで目を通してから、もう一度読むことをお勧めします。
私が理解している`PolyMesh`(ポリゴンメッシュ)と`Xform`(メッシュの変形)に関連する項目のみ取り上げます。

また、[alembic for realtime rendering](https://i-saint.hatenablog.com/entry/2016/02/09/215542)やAlembic初期のドキュメント[alembic - AlembicPoint9UsersGuide.wiki](https://code.google.com/archive/p/alembic/wikis/AlembicPoint9UsersGuide.wiki)を読むと良いかもしれません。


## Alembicとは
Alembicはレンダリングや物理シミュレーションなどを行う3DCGソフトウェア間でやりとり出来る、
時間ごとの物体の幾何情報を保存したオープンソースのファイル形式である。


## ライブラリの構成
Alembicは以下のように機能毎にディレクトリと名前空間が分けられている。
以下の表に必要なものだけを抜粋した。

|名前空間|内容|
|--|--|
|Abc|Alembicの基本的なインターフェイスを提供する|
|AbcCoreAbstract|時間に関するクラス以外ユーザーが見る必要はない|
|AbcCoreHDF5|HDF5をファイル形式に使った場合の入出力の実装|
|AbcCoreLayer|Alembicファイルへの変更を別のAlembicファイルに書き込んだり、複数のAlembicファイルを読み込んで一つのAlembicファイルにまとめる機能の実装|
|AbcCoreOgawa|Ogawaをファイル形式に使った場合の入出力の実装|
|AbcGeom|Abcを使って、物体の幾何情報や剛体変形を実装している|


### ファイルの入出力について
OgawaはHDF5よりシングルスレッドだと平均で5倍、マルチスレッドだと同じ操作を行った場合と比較して25倍速く読み込む。
また、ファイルサイズが平均で5-15%小さくなり、小さいオブジェクトが多いほどファイルサイズも小さくなる。
特に理由がなければ、Ogawaを使うことをお勧めする。


### AbcCoreLayerで出来ること
以下の表に列挙する。

|出来ること|例|
|--|--|
|形状に追加のプロパティを追加できる|UVがないポリゴンメッシュにUVを追加|
|プロパティの上書き|ポリゴンメッシュ上の静的な点を動く点に変更|
|新しいオブジェクトの追加|新しい形状を既存の階層に追加|
|オブジェクトの削除|好みじゃない形状や階層のブランチ全体の削除|
|プロパティの削除|不正な法線の削除|
|オブジェクト階層の置き換え|分けられた曲線のグループを一つの大きな曲線に置き換える|
|プロパティ階層の置き換え|ユーザープロパティ内のプロパティを他のプロパティに置き換える|

また、Alembicファイルは追記が出来ないため、`Points`など時系列で保存できない場合は時系列順にファイルに名前を付けて、この機能で読み込むと良い。

AbcCoreLayerは`MetaData`を適切に設定するために、`Util.h`内に`SetPrune`と`SetReplace`という便利な関数を提供しているらしい。


## 主な概念
Alembicでは、入力時に使うクラスには`I`が、出力時に使うものには`O`が名前の先頭に付く。
ここでは区別する必要がないため、省略する。


### `Archive`
ディスク上の実際のファイル。
すべてのシーンデータを持つトップレベルのコンテナ。
1つの`Object`を持つ。


### `Object`
Alembicの階層の主な単位。
`Object`は`Archive`の`Object`をルートとした多分木になっており、親と子供たちを取得することが出来る。
1つの`CompoundProperty`を持つ。
`ObjectHeader`と呼ばれるメタデータを持ち、`SchemaObject`を判定するために使われる。


### `Schema`
ポリゴンメッシュような、ある複雑なオブジェクトを実装するために作られた`CompoundProperty`。


#### `SchemaObject`
`Schema`を`CompundProperty`として持つ`Object`のこと。


### Property
SimpleとCompoundの2種類の型がある。
値を取得するには`SampleSelector`という、ある時点の時刻を表すクラスを与える必要がある。


#### Simple Property
ScalarとArrayの2種類ある。


##### `ScalarProperty`
型と要素の数が固定され、コンパイル時に決まる複数のSampleを持つ。
例として、以下がある。

- FloatProperty (各Sampleは32ビットの浮動小数点数; 長さ 1)
- StringProperty (各Sampleは任意の長さを持つ、1つのstring; 長さ 1)
- M44dProperty (各Sampleは16個の64ビットの浮動小数点数; 長さ 16)
- 以下の3つのSampleを持つProperty
    - 剛体の質量(1つの数値; 長さ 1)
    - 色(値が浮動小数点数のRGBのベクトル;長さ 3)
    - バウンディングボックス(値が浮動小数点数の2つの3次元ベクトル; 長さ 6)

ScalarPropertyの最大の長さは256である。


##### `TypedArrayProperty`
時刻毎の`Sample`を持つ`Property`。
`ArrayProperty`もあるが、主に使うのはこっちの方。
テンプレートパラメータに値の型(正確には[TypedPropertyTraits.h](https://github.com/alembic/alembic/blob/master/lib/Alembic/Abc/TypedPropertyTraits.h)で定義された型)が入っている。
例として、以下がある。

-  `DoubleArrayProperty` (各時刻で、要素が1つの64ビットの浮動小数点数の配列を持つ)
-  `V3fArrayProperty` (各時刻で、要素が1つの`Imath::Vec3f`(3つの32ビットの浮動小数点数)の配列を持つ)
-  `M44fArrayProperty` (各時刻で、要素が1つの`Imath::M44f`(16個の32ビットの浮動小数点数)の配列を持つ)
- 多角形メッシュの頂点のリスト
- 流体シミュレーションの粒子のリスト

ただし、同じSampleを保存しない。


#### `CompoundProperty`
(`CompoundProperty`を含む)複数のPropertyを持つProperty。


### Sample
ある時刻の生のデータ。
Propertyと同様に、SampleもScalarとArrayの2種類ある。
例として、以下がある。

- `DoubleArraySample` (要素が1つの64ビットの浮動小数点数の配列)
- `V3fArraySample` (要素が1つの`Imath::Vec3f`(3つの32ビットの浮動小数点数)の配列)
- `M44fArraySample` (要素が1つの`Imath::M44f`(16個の32ビットの浮動小数点数)の配列)


### `TimeSampling`
Propertyが持つ時間を管理するクラス。
時間の配列と`TimeSamplingType`を持つ。
`TimeSamplingType`の種類は以下の通り。

|TimeSamplingType|意味|
|--|--|
|Uniform (一様)|Sample間の時間間隔が一定|
|Cyclic (周期的)|Sample間の時間間隔が周期的に変化する|
|Acyclic (不規則)|Sample間の時間間隔が不規則に変化する|


### `SampleSelector`
Property内のSampleを取得するために使うクラス。
`TimeSampling`が持つ時間の配列のインデックスを直接与えるか、時刻を与えることで作る。
時刻を渡す場合、追加で`TimeIndexType`フラグを渡すこともできる。
`TimeIndexType`フラグの意味は以下の通り。

|TimeIndexType|意味|
|--|--|
|kNearIndex|与えられた時刻に最も近い時刻のインデックス|
|kFloorIndex|与えられた時刻より大きくない、最大の時刻のインデックス|
|kCeilIndex|与えられた時刻より小さくない、最小の時刻のインデックス|


## 主なスキーマオブジェクト
### PolyMesh
ポリゴンメッシュを管理するスキーマオブジェクト。
時刻毎に頂点、面、頂点や面の法線を持ち、UV、頂点ごとの速度、AABBも持つことが出来る。

#### PolyMeshSchema
`PolyMesh`のスキーマで、`PolyMesh`クラスの`getSchema`メンバ関数で取得できる。

このクラスの`getTopologyVariance`メンバ関数でポリゴンメッシュが変化するか判定できる。
以下は戻り値とその意味の表である。
戻り値は名前空間`Alembic::AbcGeom::MeshTopologyVariance`を省略している。

戻り値 | 意味
--- | ---
kConstantTopology | 変化しない
kHomogen(e)ousTopology | 頂点の位置は変わるが、トポロジーは変化しない
kHeterogen(e)ousTopology | 頂点の位置もトポロジーも変化する


`XformSchema`と同じように、`getValue`メンバ関数である時刻のポリゴンメッシュを取得できるが、いくつか注意点がある。

##### 面
Alembicでは、任意の多角形を持つことが出来る。
つまり、三角形や四角形を持つポリゴンメッシュをAlembicに保存できる。
このことを実現するために、面を以下のリストで表現している。

1. それぞれの面を構成する頂点のインデックスを持つ**1次元**のリスト
2. それぞれの面に含まれる頂点の個数のリスト

1つ目のリストは面のインデックスとリストのインデックスを対応させた2次元のリストをフラットにしたものでもある。
具体例として、三角形と四角形をそれぞれ1つずつ持ち、1辺を共有するメッシュは以下の通り。

1. [0, 1, 2, 1, 3, 4, 2]
2. [3, 4]

1つ目のリストを持つPropertyは`getFaceIndicesProperty`メンバ関数で取得でき、2つ目のリストを持つPropertyは`getFaceIndicesProperty`メンバ関数で取得できる。

##### 法線
実は法線とUVはこれまで説明してきたクラスのどれでもない。
法線は`TypedGeomParam`というSchemaに似たようなクラスのインスタンスで、`PolyMeshSchema`クラスの`getNormalsParam`メンバ関数で取得できる。
法線の値の意味は`TypedGeomParam`クラスの`getScope`の戻り値によって異なり、以下の通り。

|戻り値|意味|
|---|---|
|kConstantScope|ポリゴンメッシュ全体で1つだけの値|
|kUniformScope|各多角形に対して1つの値|
|kVaryingScope, kVertexScope|各多角形の頂点に対して1つの値があり、多角形内で線形に補間される|
|kFacevaryingScope|`kVaryingScope`と同様に多角形の頂点に1つの値があり、多角形内で線形に補間されるが、多角形の頂点の値は多角形毎に異なってもよい|

法線の値の取得は以下の例を参照。


### Xform
変形を管理するスキーマオブジェクト。
子に変形させる`PolyMesh`や自身を持つ。
`Xform`が親子である場合、親が変形させた後に子供が変形させる(テストを見る限り、この変形しか見当たらなかった)。
変形の順序は`getInheritsXforms`関数で判定できる。


#### XformSchema
`Xform`のスキーマで、`Xform`クラスの`getSchema`メンバ関数で取得できる。


#### XformSample
`XformSchema`クラスの`getValue`メンバ関数で`XformSample`を取得できる。
`XformSample`は`XformOp`のリストを持つ。
`getNumOps`メンバ関数で総数を取得し、`operator[]`メンバ関数でインスタンスを取得する。

直に値を取得することもできるが、操作が追加された順に取り出さなければならないため、用心して使う事。

`getNumOpChannels`メンバ関数で自身が持つ`XformOp`の`channel`の総数がわかる。


#### XformOp
`XformOp`は平行移動、回転、拡大縮小、線形変換を表す。
操作の種類は`getType`メンバ関数で判定できる。
列挙値と意味は以下の通り。

|XformOperationType|意味|
|--|--|
|kScaleOperation|拡大縮小|
|kTranslateOperation|平行移動|
|kRotateOperation|回転|
|kMatrixOperation|行列|
|kRotateXOperation|x軸回転|
|kRotateYOperation|y軸回転|
|kRotateZOperation|z軸回転|

`XformOp`は`channel`という`double`のリストを持ち、各操作を表現するために使われる。
各操作の`channel`数は以下の通り。

|操作|`channel`数|
|--|--|
|平行移動と拡大縮小|3|
|回転|4|
|行列|16|
|軸回転|1|

行列は行優先で格納される。


## 使用例
### ポリゴンメッシュの書き込み
各ディレクトリには、そのライブラリが公開している全てをインクルードした`All.h`という名前のヘッダーがある。
なので、以下のようにインクルードすればよい。

```cpp
// Alembic Includes
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
```

次に`Archive`を作り、その`Archive`の子供として、静的なポリゴンメッシュを持つアニメーションを追加しよう。
```cpp
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
```cpp
auto abc_top = archive.getTop();
auto meshyObj = OPolyMesh(abc_top, "meshy");
```

UVと法線のSampleを作成する。
その前に、UVの名前はSampleに入れる前に付ける必要がある。
```cpp
auto& mesh = meshyObj.getSchema();
mesh.setUVSourceName("test");
```

UVと法線は`GeomParams`を使う。
`GeomParams`はインデックスがあってもなくても読み書きできる。
`kFacevaryingScope`については、参考文献の3、4を参考。
```cpp
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
```cpp
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
なので、特に何もしなくて良い。

プログラム全体は参考文献の2にある。


### ポリゴンメッシュの読み込み
インクルードするヘッダーと名前空間は前例を参照。

読み込み用アーカイブを作成する。
```cpp
auto archive_reader = Alembic::AbcCoreOgawa::ReadArchive();
auto reader_ptr = archive_reader("polyMesh1.abc");
auto archive = Alembic::Abc::IArchive(reader_ptr);
```

UV, 法線を取得する。
```cpp
auto meshyObj         = IPolyMesh(archive.getTop(), "meshy");
IPolyMeshSchema& mesh = meshyObj.getSchema();
auto N                = mesh.getNormalsParam();
auto uv               = mesh.getUVsParam();
```

頂点の座標のリスト、面を構成する頂点のインデックスのリスト、面を構成する頂点の数のリスト、AABB、もしあれば速度のリストを取得する。
```cpp
IPolyMeshSchema::Sample mesh_samp;
mesh.get(mesh_samp);
```

以下のコードで、法線のリストのポインター(正確には`N3fArraySamplePtr`)を取得できる。
```cpp
auto nsp = N.getExpandedValue().getVals();
```

各法線は以下のようにして取得できる。
```cpp
for ( size_t i = 0 ; i < nsp->size() ; ++i )
{
    std::cout << i << "th normal: " << (*nsp)[i] << std::endl;
}
```

インデックスを付けたいときは、`getExpandedValue`の代わりに`getIndexedValue`を使う。
```cpp
auto uvsamp = uv.getIndexedValue();
```

値は以下のように取得する。
```cpp
V2f uv2 = (*(uvsamp.getVals()))[2];
TESTING_ASSERT( uv2 == V2f( 1.0f, 1.0f ) );
```

頂点のリストのサイズは以下で取得できる。
```cpp
mesh_samp.getPositions()->size();
```

最初の頂点の座標は以下で取得できる。
```cpp
(*(mesh_samp.getPositions()))[0];
```

取得するだけなら、以下のように簡潔に書ける。
```cpp
mesh_samp.getPositions()->get()[0];
```

## 参考文献
1. [Introduction &mdash; Alembic 1.7.0 documentation](http://docs.alembic.io/python/examples.html#properties)
1. [alembic/lib/Alembic/AbcGeom/Tests/PolyMeshTest.cpp](https://github.com/alembic/alembic/blob/master/lib/Alembic/AbcGeom/Tests/PolyMeshTest.cpp)
1. [alembic/lib/Alembic/AbcGeom/Foundation.h](https://github.com/alembic/alembic/blob/master/lib/Alembic/AbcGeom/Foundation.h)
1. [alembic/lib/Alembic/AbcGeom/GeometryScope.h](https://github.com/alembic/alembic/blob/master/lib/Alembic/AbcGeom/GeometryScope.h)
1. [Geometric Primitives](https://renderman.pixar.com/resources/RenderMan_20/geometricPrimitives.html)
1. [Alembic/AbcGeom/GeometryScope.h](https://github.com/alembic/alembic/blob/master/lib/Alembic/AbcGeom/GeometryScope.h)
