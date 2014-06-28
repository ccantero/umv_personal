CFLAGS=-I/home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/libs/commons/commons -I/home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/src/program/src -I/home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/libs/silverstack/src -I/home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/libs/parser/parser -I/home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/libs/commons/commons/collections
OBJETOS=/home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/src/program/src/program.o /home/joaquin/tp-2014-1c-silverstack/libs/commons/Debug/libcommons.so /home/joaquin/tp-2014-1c-silverstack/libs/silverstack/Debug/libsilverstack.so
FUENTES=/home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/src/program/src/program.c /home/joaquin/tp-2014-1c-silverstack/libs/commons/commons/config.c /home/joaquin/tp-2014-1c-silverstack/libs/commmons/commons/log.c

program: $(OBJETOS)
	gcc $(OBJETOS) -o program

depend:
	makedepend $(CFLAGS) $(FUENTES)

clean:
	rm /home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/src/program/src/program.o /home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/src/program/Debug/program
