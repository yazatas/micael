# Task scheduling in micael

micael features a task scheduler called MTS, _micael Task Scheduler_. It was inspired by FreeBSD's ULE and Linux's CFS schedulers. It offers the following features:

* CPU affinity
* Multilevel feedback queue (interactive and batch processes)
* Dynamic priorities
* SMP-awereness

Each CPU gets their own run queue and the load is balanced every time a new task is created or destroyed. MTS analyzes the run time behaviour of tasks to automatically detects interactive and batch processes.

The priority queue used by MTS is implemented using a binary heap. I considered using a [B-heap](https://en.wikipedia.org/wiki/B-heap) to make it VM-friendly but the number of concurrent tasks most likely won't exceed 256 making the added complexity of B-heap unnecessary.

The scheduling subsystem is designed such that the scheduler is a black box and others interact with it through common API (add task, remove task) and everything else is done internally by the scheduler. This makes it possible to create multiple differen kind of schedulers and switch between them seamlessly.

To be honest, I know very little about scheduling but I didn't want to copy someone else's scheduler but rather wanted to make my own one. This results in a design that may horrify people who are more experienced with schedulers so my apologies. Some of the design choices are not even backed by anything other than my beginner hunches. But fear not! This scheduler will be battle-tested and its design will be refined as the time goes on and my knowledge about the subject increases.

# Details of MTS

The code is thoroughly documented (header defines the API and source documents the inner workings) so it should be used as the first source of information but below is described the basic jist of MTS.

MTS assumes that each task entering the run queue for the first time is a batch process and these processes have a static timeslice and priority. Batch processes are those who almost always use their timeslice fully without blocking. Interactive processes are those who are more often blocked and which should have higher priority. In other words, batch processes are _CPU bound_ and interactive processes are _I/O bound_. Using runtime heuristics, MTS can infer the class of each process with some accuracy.

TODO: selitä miten batch- ja interactive-prosessien skeduloint eroaa (prioriteetti/timeslice)

MTS makes following assumptions: if a process uses its timeslice _90 %_ of the time without getting blocked (ie. voluntary yielding, not preempted), the process is considered a batch process. If on the other hand a task spends at least _40 %_ of the time being blocked, it is considered an interactive process and its timeslice and priority are adjusted.

TODO: what is the range 30 - 90?

For MTS, same basic assumptions are made and time will tell how they hold up and what kind of adjustments must be made. The decision to change the level of a 
