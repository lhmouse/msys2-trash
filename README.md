# MSYS2-trash

Moves files and directories into the Recycle Bin of the current user.

Drop-in replacement for `rm`. Only one C file. Zero dependencies.

You can have this in your `~/.bashrc` like on Linux:

```sh
alias rm='trash'
```

# How to Compile

```sh
gcc -Wall -Wextra -std=c99 -O2 -g trash.c -o trash
```
