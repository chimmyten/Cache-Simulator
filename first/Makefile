all: first

first: first.c
	gcc -Wall -Werror -fsanitize=address -std=c11 -g first.c -o first -lm

clean: 
	rm -rf first