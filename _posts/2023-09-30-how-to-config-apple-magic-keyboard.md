---
layout: post
title: Apple Magic KeyboardをUbuntuで使う方法
date: 2023-09-30 15:20:00 +0900
---
参考の記事が消えた時に備えて、Appleのテンキー付き日本語Magic Keyboardの設定方法をここに残しておく。
システム言語を英語とする。

## やり方
### キーボードの設定
1. 以下のコマンドを打つ。
```
sudo dpkg-reconfigure keyboard-configuration
```

1. `Apple Aluminium (JIS)`を選択する。

1. `Japanese`を選択する。

1. `Japanese - Japanese (Macintosh) `を選択する。

1. `No toggling`を選択する。

1. `No temporary switch`を選択する。

1. `The default for the keyboard layout`を選択する。

1. `No compose key`を選択する。

1. `No`を選択する。

このままだと「かな」と「英数」の挙動が逆になっているため、Mozcの設定で無理やり逆にする。

### Mozcの設定
以下のように、キーマップを変更する。`Deactivate IME`にしているのは半角英数での入力時に変換を無効にするため。

<table>
  <thead>
    <tr>
      <th>
        Mode
      </th>
      <th>
        Key
      </th>
      <th>
        Commond
      </th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>DirectInput</td>
      <td>Eisu</td>
      <td>Set input mode to Hiragana</td>
    </tr>
    <tr>
      <td>Precomposition</td>
      <td>Eisu</td>
      <td>Set input mode to Hiragana</td>
    </tr>
    <tr>
      <td>Composition</td>
      <td>Eisu</td>
      <td>Set input mode to Hiragana</td>
    </tr>
    <tr>
      <td>Conversion</td>
      <td>Eisu</td>
      <td>Set input mode to Hiragana</td>
    </tr>
    <tr>
      <td>DirectInput</td>
      <td>Hiragana</td>
      <td>Deactivate IME</td>
    </tr>
    <tr>
      <td>Precomposition</td>
      <td>Hiragana</td>
      <td>Deactivate IME</td>
    </tr>
    <tr>
      <td>Composition</td>
      <td>Hiragana</td>
      <td>Deactivate IME</td>
    </tr>
    <tr>
      <td>Conversion</td>
      <td>Hiragana</td>
      <td>Deactivate IME</td>
    </tr>
  </tbody>
</table>

### xmodmapの設定
`.Xmodmap`に以下の行を書いて`xmodmap .Xmodmap`を実行すると、`Shift`と`_`でバックスラッシュを打てるようになる。
もちろん、`_`を押せば`_`が出る。
```
keycode 97 = underscore backslash underscore backslash
```

## 参考
1. [MacBook AirでUbuntuキーボードレイアウトを設定する](https://takacity.blog.fc2.com/blog-entry-252.html)