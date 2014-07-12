#!/bin/bash
export LD_LIBRARY_PATH=/home/utnso/Entrega-TP/libs/commons/Debug:/home/utnso/Entrega-TP/libs/parser/Debug:/home/utnso/Entrega-TP/libs/silverstack/Debug
export ANSISOP_CONFIG=/home/utnso/Entrega-TP/program.config
if [ ! -f /usr/bin/ansisop ]
then
    echo "Link Simbolico No creado"
else
	echo "Link Simbolico Ya creado"
fi
