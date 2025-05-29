cc=cc
flags=-Wall -Wextra -Werror
out=stark

.PHONY: all
all: mial.o $(out)

$(out): stark.c
	$(cc) $(flags) -o $(out) mial.o stark.c

mial.o: mial.c
	$(cc) $(flags) -c -o mial.o mial.c

.PHONY: test
test:
	./$(OUT) test.sk
