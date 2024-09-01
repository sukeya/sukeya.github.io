# perf_event_paranoidを変更できない時の対処法
この記事は`perf`を使いたいが、何らかの理由で`/proc/sys/kernel/perf_event_paranoid`を変更できない方向けです。Ubuntu 22.04で検証済みです。

## 前提
以下を仮定します。
- 特権(sudoを使える)を持っている
- perfをインストール済み

## やり方
まず、`perf`があるディレクトリに移動します。シンボリックリンクでも構いません。

```sh
# which perf
/usr/bin/perf
# cd /usr/bin
```

次に、`perf_users`というグループを作成し、このグループに`perf`を追加します。これによって、`perf`を実行できるユーザーを制限できます。

```sh
# sudo groupadd perf_users
# ls -alhF
-rwxr-xr-x  2 root root  11M Oct 19 15:12 perf
# chgrp perf_users perf
# ls -alhF
-rwxr-xr-x  2 root perf_users  11M Oct 19 15:12 perf
# chmod o-rwx perf
# ls -alhF
-rwxr-x---  2 root perf_users  11M Oct 19 15:12 perf
```

`perf`に必要な権限を与え、`perf_users`のメンバーがパフォーマンスモニタリングできるようにします。

```sh
# setcap "cap_perfmon=ep" perf
# setcap -v "cap_perfmon=ep" perf
perf: OK
# getcap perf
perf cap_perfmon=ep
```

## 解説
Linuxでは、特権をケーパビリティ(capabilities)と呼ばれる、異なる単位に分割していて、非特権ユーザーのプロセスやファイルに対してスレッド毎に個別に有効または無効に出来ます。

`CAP_PERFMON`というケーパビリティを持った非特権プロセスはパフォーマンスモニタリングに関して特権を持つプロセスとして扱われます。`CAP_PERFMON`はパフォーマンスモニタリングのために必要な最小限の特権しか持たず、モニタリングのための安全な方法を提供します。

より詳しく調べたい時は必要な特権を指定します。例えば、`/proc/kallsyms`ファイルからカーネル空間のメモリアドレスを読みたい時は`CAP_SYSLOG`ケーパビリティを指定します。

`=ep`はファイルへのケーパビリティの設定で、スレッド起動時に`CAP_PERFMON`を有効にするという設定らしいです。

Linux v5.9以前では`CAP_SYS_PTRACE`が必要でしたが、それ以降では必要ありません。

## 参考文献
1. [Perf events and tool security](https://www.kernel.org/doc/html/latest/admin-guide/perf-security.html#privileged-perf-users-groups)
2. [capabilities - Linux のケーパビリティ (capability) の概要](https://manpages.ubuntu.com/manpages/bionic/ja/man7/capabilities.7.html)

