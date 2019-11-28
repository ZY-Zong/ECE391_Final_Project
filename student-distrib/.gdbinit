define ccc
target remote 10.0.2.2:1234
end

file bootimg
layout src

define lm
layout asm
end

define lc
layout src
end

define print_stack
p/x 32 $esp
end

define load_shell
add-symbol-file ../syscalls/shell.exe 0x8048094
end