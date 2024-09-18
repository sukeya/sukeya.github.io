# Apple Magic Keyboardの設定

参考の記事が消えた時に備えて、Appleのテンキー付き日本語Magic Keyboardの設定方法をここに残しておきます。
システム言語を英語とします。

## Ubuntu 22.04の場合
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

Mode | Key | Command
--- | --- | ---
DirectInput | Eisu | Set input mode to Hiragana
Precomposition | Eisu | Set input mode to Hiragana
Composition | Eisu | Set input mode to Hiragana
Conversion | Eisu | Set input mode to Hiragana
DirectInput | Hiragana | Deactivate IME
Precomposition | Hiragana | Deactivate IME
Composition | Hiragana | Deactivate IME
Conversion | Hiragana | Deactivate IME

設定が逆なのはかなと英数がキーボード上の位置と逆なため。

### xmodmapの設定
`.Xmodmap`に以下の行を書いて`xmodmap .Xmodmap`を実行すると、`Shift`と`_`でバックスラッシュを打てるようになる。
もちろん、`_`を押せば`_`が出る。
```
keycode 97 = underscore backslash underscore backslash
```

## Ubuntu 24.04の場合
### キーボードの設定
Ubuntu 22.04のキーボードの設定と同じように設定する。

### xmodmapの設定
かな、英数、Caps Lockが正しく認識されなかったため、`xmodmap`で以下のように変更する。

```
keycode 97 = underscore backslash underscore backslash
keycode 130 = Hiragana Hiragana Hiragana Hiragana
keycode 131 = Eisu_toggle Eisu_toggle Eisu_toggle Eisu_toggle
keycode 66 = Caps_Lock Caps_Lock Caps_Lock Caps_Lock
```

### Mozcの設定
`IBus`用の`Mozc`と`Mozc`のGUIユーティリティをインストールする。

```
sudo apt install ibus-mozc mozc-utils-gui
```

その後、設定のキーボードの「Add Input Source...」から「Japanese (Mozc)」を追加する。
そして、以下のようにキーマップを変更する。

Mode | Key | Command
--- | --- | ---
DirectInput | Hiragana | Set input mode to Hiragana
Precomposition | Hiragana | Set input mode to Hiragana
Composition | Hiragana | Set input mode to Hiragana
Conversion | Hiragana | Set input mode to Hiragana
DirectInput | Eisu | Deactivate IME
Precomposition | Eisu | Deactivate IME
Composition | Eisu | Deactivate IME
Conversion | Eisu | Deactivate IME

## 参考
1. [MacBook AirでUbuntuキーボードレイアウトを設定する](https://takacity.blog.fc2.com/blog-entry-252.html)
