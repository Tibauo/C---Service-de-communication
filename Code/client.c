#include <curses.h>	
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

WINDOW* TEST;
static int remote;

struct screen {
	WINDOW* upper;
	WINDOW* lower;
	WINDOW* footer;
};

struct params {
	char* host;
	int port;
};

int print(WINDOW* win, char* string){
	if(win == NULL){
		// flag debug
		fprintf(stderr, "print: get a NULL window\n");
		return(1);
	}
	
	wprintw(win, string);
	wrefresh(win);
	return(0);
}

struct screen init_window(){
	WINDOW *L1, *L2;
	struct screen result;
 	int SZ; /* taille d'un cadre */
	
	result.upper = NULL;
	result.lower = NULL;
	result.footer = NULL;
	
 	initscr(); // Initialisation (nettoyage) de l'ecran
	// printw("%i LINES %i COLS \n ", LINES, COLS);
	refresh();
	
	SZ = (LINES-2)/3;
	if(SZ < 6 || COLS < 12){
		endwin();
		fprintf(stderr, "ERROR : Fenetre trop petite \n");
		return(result);
	}
	
	// creation de trois cadres superposes separes par une ligne 
 	result.upper = newwin(SZ, COLS, 0, 0); 
	L1 = newwin( 1, COLS, SZ, 0);
	result.lower = newwin( SZ, COLS, SZ+1, 0);
 	L2 = newwin( 1, COLS, (2*SZ)+1, 0);
	
 	result.footer = newwin( LINES-(2*SZ)-2, COLS,2+(2*SZ), 0);
	/* pas de curseur */
 	leaveok(result.lower, TRUE);
 	leaveok(result.footer, TRUE);
 	leaveok(L1, TRUE);
 	leaveok(L2, TRUE);
	leaveok(result.upper, TRUE);
	
	// scroll automatique sur W1, W2, C
	scrollok(result.upper, TRUE);
	scrollok(result.lower, TRUE);
	scrollok(result.footer, TRUE);
	
	// tracer les lignes
 	whline(L1, '=', COLS);
 	whline(L2, '=', COLS);
	wrefresh(L1);
	wrefresh(L2);
	
	
	// recuperation des caracteres tels que frappes
    cbreak();
	//pas d'echo automatique
	noecho();
	
	return(result);
}

//affiche le cas ou l'utilisation est mauvaise
void usage(char* program){
	fprintf(stderr, "Usage: %s -s host -p port\n", program);
	fprintf(stderr, "\t-p : numéro de port\n");
	fprintf(stderr, "\t-s : adresse du serveur\n");
	fprintf(stderr, "\t-h : print help and exit\n");
}

// gestion des paramètres
struct params getparams(int argc, char** argv){
	int c;
	struct params result;
	
	if((argc != 5) && (argc!=1)){
		usage(argv[0]);
		exit(1);
	}
    //host et port par défaut
	if (argc==1) {
        result.host= "127.0.0.1";
        result.port=30000;
    } else {
        while ((c = getopt(argc , argv, "s:p:")) != -1){
            switch (c){
                case 's':
                    result.host = optarg;
                    break;
                case 'p':
                    result.port = atoi(optarg);
                    break;
                default:
                    usage(argv[0]);
                    exit(1);
                    break;
            }
        }
    }
	
	printf("serveur : %s\n", result.host);
	printf("port : %i\n", result.port);
	return(result);
}

int join(char* serveur_name,int port){
	struct sockaddr_in serverSockAddr;
	struct hostent *serverHostEnt;
	long hostAddr;
	int to_server_socket = -1;
	
	bzero(&serverSockAddr,sizeof(serverSockAddr));
	hostAddr = inet_addr(serveur_name);
	
	if ( (long)hostAddr != (long)-1)
		bcopy(&hostAddr,&serverSockAddr.sin_addr,sizeof(hostAddr));
	else{
		serverHostEnt = gethostbyname(serveur_name);
		if (serverHostEnt == NULL){
            printf("gethost rate\n");
			exit(1);
		}
		bcopy(serverHostEnt->h_addr,&serverSockAddr.sin_addr,serverHostEnt->h_length);
	}
	serverSockAddr.sin_port = htons(port);
	serverSockAddr.sin_family = AF_INET;
	
	//creation de la socket
	if ( (to_server_socket = socket(AF_INET,SOCK_STREAM,0)) < 0){
		printf("creation socket client ratee\n");
		exit(1);
	}
	// requete de connexion
	if(connect( to_server_socket,
			   (struct sockaddr *)&serverSockAddr,
			   sizeof(serverSockAddr)) < 0 ){
		printf("demande de connection ratee\n");
		exit(0);
	}
	
	remote = to_server_socket;
	return(to_server_socket);
}

//Permet d'enregistrer la texter saisi par l'utilisateur
void handler(char* line){
	write(remote, line, strlen(line));
	write(remote, "\n", 1);
}

//éteind tout
void bye(){
	endwin();
	shutdown(remote, SHUT_RDWR);
	close(remote);
}


int main(int argc, char* argv[]){
	struct screen win;
	struct params params;
	int fd;
	fd_set fds;
	int i;
	int nbchar;
	char buffer[512];
	
	params = getparams(argc, argv);
	
	win = init_window();
	if(win.upper == NULL || win.lower == NULL){
		fprintf(stderr, "Windows initialisation failed\n");
		return(1);
	}
	TEST = win.upper;
	// Si erreur d'execution alonrs on ferme tout
	i = atexit(bye);
	if(i != 0){
		fprintf(stderr, "atexit init failed: %m\n");
		endwin();
		exit(1);
	}
	
	fd = join(params.host, params.port);
	rl_callback_handler_install("$ ", (char*) &handler);
	
    //tant que l'utilisateur ne ferme pas
	while(1){
		FD_ZERO(&fds);
	
		FD_SET(1, &fds);
		FD_SET(fd, &fds);
		
		i = select(fd + 1, &fds, NULL, NULL, NULL);
		if(i == -1){
			perror("select");
			goto error;
		}
		
		//affiche
		if(FD_ISSET(1, &fds)){
			rl_callback_read_char();
			werase(TEST);
			mvwchgat(TEST, 0, rl_point, 1, A_REVERSE, 0, NULL);
			mvwprintw(TEST, 0, 0, "%s", rl_line_buffer);
			wrefresh(TEST);
		}
		
		//socket
		if(FD_ISSET(fd, &fds)){
			memset(buffer, 0, 512);
			nbchar = read(fd, buffer, 512);
			if(nbchar == -1){
				perror("read from server failed");
				goto error;
			}
			print(win.lower, buffer);
			
		}
	}
	
error:
	exit(0);
	endwin();
	return(1);
}
