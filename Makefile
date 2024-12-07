LD := $(CC)
CC_FLAGS := -Isrc/tester/ -g -Og -fsanitize=undefined -fsanitize=address
LD_FLAGS := -fsanitize=undefined -fsanitize=address
all: tester
bin/slobos.o: src/slobos.c
	$(CC) -c $(CC_FLAGS) src/slobos.c -o bin/slobos.o
bin/main.o: src/tester/main.c
	$(CC) -c $(CC_FLAGS) src/tester/main.c -o bin/main.o
tester: bin/main.o bin/slobos.o
	$(LD) -otester $(LD_FLAGS) bin/main.o bin/slobos.o
clean:
	rm bin/*
