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

define print-stack
p/x 32 $esp
end