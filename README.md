# Cherysh: A UNIX Shell with a Stack VM and Meta-programming


So, I am currently working on 3 languages. This is one of them. The other one is [Squawk](https://github.com/Chubke/Squawk) which is pretty fall along, and the other one is JigCC --- a C compiler written in Java with my own IR. JigCC is still at infancy. This project, Cherysh, is not as well-progressed as Squawk is, but there's still a good amount of functions defined inside `cherysh.c`. I have already added the quoting system. I have added the exec-fork-wait loop as well. I have added file dupping as well. So a lot of 'basic' Shell stuff are already defined.

After I push this, if I don't decide to work on Squawk or JigCC, I will create a new file called `parallel-portable.h` and `parallel-linux.h`. There, I will add parallel process launching and multithreading and stuff. I might create `parallel-linux.h` first. In `parallel-linux.h` I plan on creating my own threading library using `clone(2)` and `sched(7)`.

Thanks to `u/NextYam3704` who suggested me to share the repository, otherwise I would not have remoted it!


