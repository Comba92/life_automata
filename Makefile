TITLE = life

CC = gcc
CFLAGS = -Wall -Wextra -Wshadow -O3 -std=c17 -o ${TITLE}
CONSOLE = -mwindows
PATHS = -IC:\mingw64\include -LC:\mingw64\lib 
LINKER = -lraylib -lopengl32 -lgdi32 -lwinmm

dev: 
	${CC} ${CFLAGS} ${PATHS} main.c ${LINKER} 
	./${TITLE}

prod: 
	${CC} ${CFLAGS} ${PATHS} ${CONSOLE} main.c ${LINKER} 

clean:
	rm -f ${TITLE}.exe