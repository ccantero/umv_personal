CFLAGS=-I/home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/libs/commons/commons -I/home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/src/umv/src -I/home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/libs/silverstack/src -I/home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/libs/parser/parser -I/home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/libs/commons/commons/collections
OBJETOS=/home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/src/umv/src/umv.o /home/joaquin/tp-2014-1c-silverstack/libs/commons/Debug/libcommons.so /home/joaquin/tp-2014-1c-silverstack/libs/silverstack/Debug/libsilverstack.so /home/joaquin/tp-2014-1c-silverstack/libs/parser/Debug/libparser.so
FUENTES=/home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/src/umv/src/umv.c

umv: $(OBJETOS)
	gcc $(OBJETOS) -o umv -lncurses -lpthread

depend:
	makedepend $(CFLAGS) $(FUENTES)

clean:
	rm /home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/src/umv/src/umv.o /home/joaquin/Back-Up-TP/tp-2014-1c-silverstack/src/umv/Debug/umv
