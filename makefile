all: test01 test02 test03

test01: apps/test01.c io/system.c io/file.c io/disk.c
	gcc -o test01 apps/test01.c io/system.c io/file.c io/disk.c -std=c99

test02: apps/test02.c io/system.c io/file.c io/disk.c
	gcc -o test02 apps/test02.c io/system.c io/file.c io/disk.c -std=c99

test03: apps/test03.c io/system.c io/file.c io/disk.c
	gcc -o test03 apps/test03.c io/system.c io/file.c io/disk.c -std=c99