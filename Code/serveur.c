#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

void fserveur(char* serveur_name, int port){
	char buffer[512];
	int ma_socket;
	int client_socket;
	struct sockaddr_in mon_address, client_address;
	socklen_t socklen;
	fd_set saved_fds;
	fd_set run_fds;
	int maxfd;
	int i = 1;
	int res;
	int ready_fd;
	int j;
	
	bzero(&mon_address,sizeof(mon_address));
	mon_address.sin_port = htons(port);
	mon_address.sin_family = AF_INET;
	mon_address.sin_addr.s_addr = htonl(INADDR_ANY);
	
	/* creation de socket */
	if ((ma_socket = socket(AF_INET,SOCK_STREAM,0))== -1){
		printf("la creation rate\n");
		exit(0);
	}
	
	setsockopt(ma_socket, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int));

	   
	//Serveur socket
	bind(ma_socket,(struct sockaddr *)&mon_address,sizeof(mon_address));
	
	//écouter sur le réseau
	listen(ma_socket, 10);
	
	//On initialise les infos (nombre de personne connecté,....)
	FD_ZERO(&saved_fds);
	maxfd = ma_socket;
	FD_SET(ma_socket, &saved_fds);
	
    //Bouclie infinie, en effet le serveur devra toujours fonctionner
	while(1){
		memcpy(&run_fds, &saved_fds, sizeof(saved_fds));
		res = select(maxfd + 1, &run_fds, NULL, NULL, NULL);
		if(res == -1){
			perror("select failed");
			break;
		}

		ready_fd = res;
		for(i = 1; i <= maxfd && ready_fd > 0; i++){
			if(!FD_ISSET(i, &run_fds))
				continue;
			
			ready_fd = ready_fd - 1;
			//gère la partie acceptation
            if(i == ma_socket){
				client_socket = accept(i,
									   (struct sockaddr *)&client_address,
									   &socklen);
				if(client_socket == -1){
					perror("accept failed");
					break;
				}
				//On ajoute le client à la "liste du serveur"
				fprintf(stderr, "DEBUG: incoming connection: %d\n", client_socket);
				FD_SET(client_socket, &saved_fds);
				if (client_socket > maxfd)
					maxfd = client_socket;
				
				break;
			}
			
            //Ici on gère la lecture
			res = read(i, buffer, 512);
			if(res < 0){
				perror("read failed");
				break;
			}
			
			//Fermeture de connexion
			if(res == 0){
				fprintf(stderr, "connection %i closed\n", i);
				close(i);
				FD_CLR(i, &saved_fds);
				if (i == maxfd){
					while (FD_ISSET(maxfd, &saved_fds) == 0)
						maxfd = maxfd - 1;
				}
			} else {
                //on affiche sur les clients
				fprintf(stderr, "read %i bytes\n", res);
				for(j = 3; j <= maxfd; j++){
					if(j != ma_socket)
						write(j, buffer, res);
				}
			}
		}
	}
}



int main(int argc, char *argv[]){
	int c;
	char *serveur = "127.0.0.1";
	int port = 30000;
	while ((c = getopt(argc , argv, "s:p:")) != -1)
		switch (c) 
		{
			case 's':
				serveur=optarg;
				break;
			case 'p':
				port=atoi(optarg);
				break;
			default:
				printf("./client [-ps]\n");
				printf("-p : numero de port\n");
				printf("-s : adresse du serveur\n");
				break;
		}
 
	printf("serveur : %s \n", serveur);
	printf("port : %d \n", port);
		
	fserveur(serveur, port);
	
	
	return 0;
}

