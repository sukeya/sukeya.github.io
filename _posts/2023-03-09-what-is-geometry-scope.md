---
layout: post
title:  Geometry Scopeとは
date:   2023-03-09 16:20:00 +0900
---
「[Alembicの使い方]({% post_url 2023-02-28-how-to-use-alembic %})」のポリゴンメッシュの読み込みの例で、UVマップと法線のSampleの作成で`GeometryScope`を指定した。
指定方法の参照先として参考文献にRenderManのURLを載せたが、私自身も使わざるを得なくなり、「Alembicの使い方」が肥大化するのを防ぐため新たにポストを作ることにした。

## 結論
`Polygon`に対しては以下の意味となる。

|フラグ|意味|
|--|--|
|kConstantScope|`Polygon`全体で1つだけの値|
|kUniformScope|各多角形に対して1つの値|
|kVaryingScope, kVertexScope|各多角形の頂点に対して1つの値があり、多角形内で線形に補間される|
|kFacevaryingScope|kVaryingScopeと同様に多角形の頂点に1つの値があり、多角形内で線形に保管されるが、多角形の頂点の値は多角形毎に異なってもよい|

## 余談
`GeneralPolygon`とは、穴と凹みを持つ多角形のリストである。
例えば、ブーメランのようにある内角が180度を超えている多角形やアメリカ国防総省の建物のように多角形の穴が空いている多角形である。

## 参考
1. https://renderman.pixar.com/resources/RenderMan_20/geometricPrimitives.html
2. https://github.com/alembic/alembic/blob/master/lib/Alembic/AbcGeom/GeometryScope.h
