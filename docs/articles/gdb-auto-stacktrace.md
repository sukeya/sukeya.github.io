#

```cpp title="main.cpp"
#include <cassert>

int main() {
	assert(false);
}
```

## gdb

``` title="cmds.txt"
catch signal SIGABRT
commands 1
  bt
end
r
q
```

```sh
gdb -x cmds.txt -q ./a.out
```

## Microsoft console debugger
``` title="cmds.txt"
.sympath srv*
.reload
sxe -c "kp" asrt
g
q
```

```
cdb -cfr cmds.txt -kqm main.exe
```

https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/getting-started-with-windbg
https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/controlling-exceptions-and-events
https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/k--kb--kc--kd--kp--kp--kv--display-stack-backtrace-
https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/sx--sxd--sxe--sxi--sxn--sxr--sx---set-exceptions-
https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/cdb-command-line-options