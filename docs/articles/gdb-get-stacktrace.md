# gdbでのスタックトレースの自動取得

C++では、セグメンテーション違反やアサーションの失敗時のスタックトレースを取るにはデバッガーを使う必要があり、通常インタラクティブに操作する必要があります。
この記事では、予めブレークポイント(正確にはキャッチポイント)を設定しておくことでこれらが発生したときにスタックトレースを出力して終了する方法を紹介します。
GitHub ActionsなどCIで使うと便利だと思います。

例として、以下の単純なプログラムを考えます。

```cpp title="fail_assert.cpp" linenums="1"
#include <cassert>

int main() {
	assert(false);
}
```

次に、以下のようなgdbのコマンドを書いたテキストファイルを用意します。

``` title="cmds.txt" linenums="1"
set pagination off
catch signal SIGABRT SIGSEGV
commands 1
  bt
end
r
q
```

1行目はページ区切りをオフにしています。
`catch signal`でセグメンテーション違反とアサーションの失敗時に出るシグナルを捕まえます。
`commands 1`〜`end`の部分は直前に設定したキャッチポイントの番号を指定して、シグナルを捕まえた時にスタックトレースを取得するよう指示しています。
そして、以下のコマンドを実行します。

```sh
g++ -g -O0 fail_assert.cpp -o fail_assert
gdb -x cmds.txt -q fail_assert
```

`-x`にgdbのコマンドが書かれたテキストファイルを指定することで、そのファイル内のコマンドを順に実行させます。
また、`-q`で不要な出力を消しています。
実行すると、以下のようなスタックトレースを含んだ出力が得られるはずです。

```
Reading symbols from fail_assert...
Catchpoint 1 (signals SIGABRT SIGSEGV)

[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".
fail_assert: fail_assert.cpp:4: int main(): Assertion `false' failed.

Catchpoint 1 (signal SIGABRT), __pthread_kill_implementation (no_tid=0, signo=6, threadid=<optimized out>)
    at ./nptl/pthread_kill.c:44
warning: 44     ./nptl/pthread_kill.c: No such file or directory
#0  __pthread_kill_implementation (no_tid=0, signo=6, threadid=<optimized out>) at ./nptl/pthread_kill.c:44
#1  __pthread_kill_internal (signo=6, threadid=<optimized out>) at ./nptl/pthread_kill.c:78
#2  __GI___pthread_kill (threadid=<optimized out>, signo=signo@entry=6) at ./nptl/pthread_kill.c:89
#3  0x00007ffff7c4526e in __GI_raise (sig=sig@entry=6) at ../sysdeps/posix/raise.c:26
#4  0x00007ffff7c288ff in __GI_abort () at ./stdlib/abort.c:79
#5  0x00007ffff7c2881b in __assert_fail_base (fmt=0x7ffff7dd01e8 "%s%s%s:%u: %s%sAssertion `%s' failed.\n%n", 
    assertion=assertion@entry=0x55555555601f "false", file=file@entry=0x55555555600f "fail_assert.cpp", 
    line=line@entry=4, function=function@entry=0x555555556004 "int main()") at ./assert/assert.c:94
#6  0x00007ffff7c3b507 in __assert_fail (assertion=0x55555555601f "false", 
    file=0x55555555600f "fail_assert.cpp", line=4, function=0x555555556004 "int main()")
    at ./assert/assert.c:103
#7  0x0000555555555179 in main () at fail_assert.cpp:4
A debugging session is active.

        Inferior 1 [process 10899] will be killed.

Quit anyway? (y or n) [answered Y; input not from terminal]
```

ここではコマンドを列挙しただけでしたが、if文やwhile文も使えるのでより複雑なことも出来ます。

## 参考文献
1. [5.1.3 Setting Catchpoints](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Set-Catchpoints.html)
2. [23.1.3 Command Files](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Command-Files.html)
