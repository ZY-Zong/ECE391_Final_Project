file bootimg
layout src
focus cmd

define ccc
target remote 10.0.2.2:1234
end

define lm
layout asm
end

define lc
layout src
end

define p_stack
x/32x $esp
end

define load_shell
add-symbol-file ../syscalls/shell.exe 0x8048094
end

define load_ls
add-symbol-file ../syscalls/ls.exe 0x8048094
end

define p_run
p *running_task()
end

define p_focus
p *focus_task()
end

define p_buf
p (uint8_t *) buf
end

define restore_stack
set $esp = $ebp
end