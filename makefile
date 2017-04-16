default: ./lab5.c
	gcc -o lab5.run ./lab5.c -lm

test: ./lab5.c
	reset
	echo gcc:
	gcc -o lab5.run ./lab5.c -lm -fsanitize=address
	echo cppcheck:
	cppcheck --enable=all --inconclusive --std=posix lab5.c
	echo codepatch.pl:
	/home/batya/linux/scripts/checkpatch.pl -f /home/batya/labs/lab5/lab5.c
