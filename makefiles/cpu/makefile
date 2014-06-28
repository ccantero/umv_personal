LIBS := -lpthread -pthread -lcommons -lsilverstack -lparser

SILVER_I := -I"/home/utnso/Escritorio/tp-2014-1c-silverstack/libs/silverstack/" 
COMMONS_I := -I"/home/utnso/Escritorio/tp-2014-1c-silverstack/libs/commons/"
PARSER_I := -I"/home/utnso/Escritorio/tp-2014-1c-silverstack/libs/parser/"

SILVER_L := -L"/home/utnso/Escritorio/tp-2014-1c-silverstack/libs/silverstack/Debug"
COMMONS_L := -L"/home/utnso/Escritorio/tp-2014-1c-silverstack/libs/commons/Debug"
PARSER_L := -L"/home/utnso/Escritorio/tp-2014-1c-silverstack/libs/parser/Debug"

make:
	gcc -Wall cpu.c $(LIBS) $(SILVER_I) $(COMMONS_I) $(PARSER_I) $(SILVER_L) $(COMMONS_L) $(PARSER_L) -o cpu    
clean:
	rm $($PROG) cpu
	
