LIBS := -lpthread -pthread -lcommons -lsilverstack -lparser

SILVER_I := -I"/home/utnso/Entrega-TP/libs/silverstack/" 
COMMONS_I := -I"/home/utnso/Entrega-TP/libs/commons/"
PARSER_I := -I"/home/utnso/Entrega-TP/libs/parser/"

SILVER_L := -L"/home/utnso/Entrega-TP/libs/silverstack/Debug"
COMMONS_L := -L"/home/utnso/Entrega-TP/libs/commons/Debug"
PARSER_L := -L"/home/utnso/Entrega-TP/libs/parser/Debug"

make:
	gcc -Wall ../src/kernel.c ../src/protocol.c $(LIBS) $(SILVER_I) $(COMMONS_I) $(PARSER_I) $(SILVER_L) $(COMMONS_L) $(PARSER_L) -o kernel    
clean:
	rm $($PROG) kernel
	
