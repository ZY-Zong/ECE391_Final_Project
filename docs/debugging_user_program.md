# Add New User Program

Firstly, write code of new user program in `syscalls` folder.

Then with `-g` turned off in `syscalls/Makefile`, in `syscalls` folder, run:
```shell script
make mytest
```

Then copy the compiled user program and build the new file system.
```shell script
cd .. # You should now be in your mp3 directory
cp syscalls/to_fsdir/mytest fsdir/
./createfs -i fsdir -o student-distrib/filesys_img
```

# Compile Symbol Files of User Programs

Add `-g` to `CFLAGS` and `LDFLAGS` in `syscalls/Makefile`. The first two lines should look like
```makefile
CFLAGS += -g -Wall -nostdlib -ffreestanding
LDFLAGS += -g -nostdlib -ffreestanding
```

Then compile the symbol file. For example:
```shell script
make ls.exe
```

To load the file into gdb:
1. In the `syscalls` folder, run `readelf -S ls.exe`
2. Look for a line with name `.text`, make a note of its address. It should be something like `08048094`
3. In the `student-distrib` folder, run `gdb bootimg` as you would normally
4. In gdb, run `add-symbol-file ../syscalls/ls.exe 0x8048094`. Enter Y when prompted to confirm.

`.gdbinit` file provides some definitions to load some user program symbols.

# Fix Encoding

```shell script
dos2unix *.c
```

# Reference
[Post by Fang Lu on Piazza](https://piazza.com/class/jzjux8xiyir48d?cid=957)