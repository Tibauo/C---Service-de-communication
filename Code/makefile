#!/usr/bin/make -f
CC=gcc
CFLAGS=-Wall -g
LDFLAGS=
EXEC=server client

all : 
	$(CC) $(CFLAGS) client.c -o client -L/usr/ccs/lib -L/usr/ucblib -lcurses -ltermcap -lreadline && $(CC) $(CFLAGS) serveur.c -o server
	@echo ""		
	@echo "Voici le projet C de Thibaut Di Prima"
	@echo "Le fichier serveur est compile pour l'executer : ./serveur -p numerodeport -s adresseserveur"
	@echo "Le fichier client compile pour l'executer : ./client -p numerodeport -s adresseserveur"
	@echo ""

mrproper : 
	@echo ""	
	@rm -rf $(EXEC)
	@rm -rf *.dSYM
	@echo "Les executables ont etes supprime"
	@echo ""
