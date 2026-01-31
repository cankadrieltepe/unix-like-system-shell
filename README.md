# stash — Unix-Style Shell (COMP 304 Project 1)

`stash` is an interactive Unix-style shell implemented in **C** for **COMP 304 – Operating Systems (Fall 2025)**.  
It reads commands from **stdin** (interactive mode) or from a **.sh script file** (script mode) and executes them using `fork()` + `execv()` (with manual PATH resolution).

## Features (Project Requirements)
- Built-in commands implemented inside `stash`
- Execute external Linux programs via `fork()` + `execv()` (no `exec*p`)
- Script execution: run commands line-by-line from a `.sh` file
- Auto-complete with `TAB` using executables from `PATH`
- I/O redirection: `>`, `>>`, `<`
- Piping: `cmd1 | cmd2 | cmd3 | ...`
- Command history (arrow keys + `history` built-in)
- `lsfd <PID>` built-in using a **Linux kernel module** + `/proc` communication  
  - `stash` loads the module at runtime if not loaded, and unloads it on exit if it is the last instance

My references

https://medium.com/@deeppadmani98.2021/understanding-the-proc-file-system-in-linux-90746e3ba76a
I used this to get more knowledge on proc file system and used it to learn how the basics of the code works.

https://unix.stackexchange.com/questions/332948/how-does-lookup-in-path-work-under-the-hood
I got knowledge from this for my path search logic.

https://www.kernel.org/doc/html/latest/RCU/whatisRCU.html
I learned rcu and the functions I can use in my project for rcu.

https://stackoverflow.com/questions/19099663/how-to-correctly-use-fork-exec-wait
I looked at this to learn more about fork exec wait logic.
