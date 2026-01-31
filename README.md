This project is an interactive Unix-style operating system shell, called stash in C. After executing stash , it will read commands from the
user via console (i.e. stdin) or a file and execute them. Some of these commands will be builtin commands, i.e., specific to stash and implemented in the same executable binary,
while it should be able to launch other commands which are available as part of your own linux system. This is a project done in my COMP304 course.

My references

https://medium.com/@deeppadmani98.2021/understanding-the-proc-file-system-in-linux-90746e3ba76a
I used this to get more knowledge on proc file system and used it to learn how the basics of the code works.

https://unix.stackexchange.com/questions/332948/how-does-lookup-in-path-work-under-the-hood
I got knowledge from this for my path search logic.

https://www.kernel.org/doc/html/latest/RCU/whatisRCU.html
I learned rcu and the functions I can use in my project for rcu.

https://stackoverflow.com/questions/19099663/how-to-correctly-use-fork-exec-wait
I looked at this to learn more about fork exec wait logic.
