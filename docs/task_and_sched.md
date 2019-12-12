# History
* Initial version. --liuzikai on Nov. 30, 2019.

# Some Terminologies
* AuroraOS supports only process, but not thread. We use term 'task' for process in AuroraOS.
* Each task will has a Process Control Block (PCB) that store info for this specific task, which is `task_t` defined in
*task.h*.
* Each task has a 8KB Process Kernel Memory (PKM), growing from 8MB (the end of kernel image in memory) to lower
address, which can be accessed through `task_slot(idx)` in *task.c*. The PCB lies on the top (low address) of PKM, and
kernel stack (stack for kernel state such as interrupts and system calls when in this task) grows from bottom (high
address). Notice that if `TASK_MAX_COUNT` is set too large, PKMs may overlap with kernel image but only a few checks 
are performed.

# `running_task()` AND `focus_task()`
* `running_task()` in *task.h* is the pointer to current running task (which interrupt happens among, or caller of system call).
Since PKMs are 8KB-aligned, we can get running_task() by align current ESP to 8KB. All system calls should use
this as caller task, and screen output should be write to corresponding video memory of this.)
* `focus_task()` is the task that is at the foreground and accept keyboard input. There is a local variable that maintain
this variable and it is changed through `task_change_focus()`. Although terminal can be shared (for example, a shell
execute cat, they share a terminal so that all content is written on the same screen,) we assume that only one
task can be the actual user of a terminal that accept keyboard input (for example, a shell execute a program and
wait for its halt, then the new program become the actual user; if the shell won't wait for its halt, it runs
as background task and the shell remains terminal user.) The mapping from terminal ID to task is maintained in the
`terminal_fg_task[]` in *task.c*.

# General Rule of Context Switch
* Firstly, remember that all context switches are performed in interrupts or system calls. No matter it was in user
state or kernel state, there is always hardware context on the stack that stores flags and all registers. As long as
the interrupts or system calls can return, flags and all registers will be recovered.
* Also as a consequence, all tasks that involves in switch are in kernel states. So, we only need to switch between 
kernel stacks instead of user stacks.
* We set the rule that paging and running terminal are set **before** stack switch and EIP switch.
* We set the rule that, before leaving current task (both execute() or scheduling), three things are pushed on the 
stack in order:
1. EFLAGS.
2. EBP. 
3. Return address when waking up this task. Restore EBP, EFLAGS and then continuous original task.
* Thereby to switch to a task:
1. Change ESP.
2. Run `ret`. Code of the task to switch to will take over.
* Context switch functions serves as normal function. Calling them will yield processor to other tasks. When the task
gets running again, it will continue as if the function return.
* Since we save flags for each task and enable IF for new task at `system_execute()` by setting flags manually,
it's save to wrap the whole context switching function in a lock, which is recommended.

# `execute()` and `halt()`
* `system_execute()` is the extended version of system call `execute()`.
* If parent waits for child to halt, it will be put into a wait list `wait4child_list` in *task.c*, and waked up after
child halt.
* If parent doesn't wait for child to halt, it won't be put into the wait list. Although processor will be yield to
new program immediately, parent task is still in the run queue. When it gets running again, `system_execute()` will 
return -1. The child has it parent set to NULL.
* `system_halt()` is also extended so that it can accept status greater than 255 (use when halt because of exceptions)
* The most important low-level context switch functions are `execute_launch()` and `halt_backtrack()` in *task.c*. See
comments there.

# Task List
* Without scheduling, the implementation of terminal and RTC `read()` is to run an infinity loop and wait for interrupts
to update status. But with scheduling, it's a waste of time if the available time for a task is used to run
infinity loop. So we implement wait lists. When a task (shall be `running_task()`, which is the caller of system calls)
is put into waiting status, it is removed from run queue and put into a corresponding wait list. When it's time
for it to return, it is inserted back to the HEAD of run queue, which make terminal more responsive and RTC more
accurate.
* Notice that both wait lists and run queue can be implemented with doubly-linked list. Also, a task can only be in
one list at a time, no matter it's run queue or a wait list. So we implement general task list (with sentinel as
list head). `task_list_node_t` is the structure for doubly-linked list node in each task_t. `task_from_node()` uses
tricks of address arithmetic to get the task_t that contains the node (inspired by linux 2.6 source, but
simplified.) Several helper functions are provided. MAKE SURE READ COMMENTS BEFORE USING THEM.