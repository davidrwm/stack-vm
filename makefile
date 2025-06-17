CC := gcc
CFLAGS := -c -O0

INCPATH := -IC:/SDL3/include -I./include
LIBPATH := -LC:/SDL3/lib

LIBS := -lmingw32 -lSDL3

OBJ :=	\
		./obj/cpu.o												\
		./obj/disk.o											\
		./obj/display.o											\
		./obj/io.o												\
		./obj/main.o											\
		./obj/memory.o											\

ssvm.exe: $(OBJ)
	$(CC) $(OBJ) -o $@ $(LIBPATH) $(LIBS)

./obj/%.o: ./src/%.c
	$(CC) $< -o $@ $(CFLAGS) $(INCPATH)

clean:
	rm ./obj/*.o