//Practica Tema 6: Bravo Torres, Elisa

#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<ctype.h>
#include<signal.h>
#include<unistd.h>
#include<errno.h>

int socketfd = 0;

int otroSock = 0;


void signal_handler(int);

int main(int argc, char ** argv){

	//Nos aseguramos que no haya mas de tres argumentos: Nombre de la funcion, -p y numero de puerto.
	if(argc > 3){
		fprintf(stderr,"La llamada debe ser de la forma: ./echocon-tcp-servidor-bravotorres [-p puerto-servidor]\n");
		exit(EXIT_FAILURE);
	}

	unsigned short puerto;

	//Si el usuario indica el puerto, lo guardamos
	if(argc ==3 && strcmp(argv[1], "-p") == 0){
		sscanf(argv[2],"%hu",&puerto);
		puerto = htons(puerto);

		if(puerto != htons(5)){
			fprintf(stderr,"El puerto debe ser el 5\n");
			exit(EXIT_FAILURE);
		}
	}
	//Si no se indica puerto, ponemos com puerto el numero 5
	else if (argc == 1){
		puerto =htons(5);
	}

	//Si el usuario no indica bien el puerto le recordamos la llamada
	else{
		fprintf(stderr, "Los parametros son: ./echocon-tcp-server-bravotorres [-p puerto_servidor]\n");
		exit(EXIT_FAILURE);
	}
	int sig = 0;
 	signal(sig, signal_handler);

	//Creamos el socket del servidor
	socketfd = socket(AF_INET, SOCK_STREAM,0);
	if(socketfd == -1){
		perror("socket()");
		exit(EXIT_FAILURE);
	}

	//Enlazamos el socket con una ip y puerto locales

	if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int))<0){
		perror("setsockopt(SO_REUSEPORT)");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = puerto;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(socketfd,(struct sockaddr *) &my_addr, sizeof(my_addr)) == -1){
		perror("bind()");
		exit(EXIT_FAILURE);
	}


	if(listen(socketfd, 6) == -1){
		perror("listen()");
		exit(EXIT_FAILURE);
	}

	//Iniciamos el bucle de espera de peticiones
	while(1){

		struct sockaddr_in client_addr;
		socklen_t tamano_client_addr = sizeof(client_addr);

		otroSock = accept(socketfd, (struct sockaddr*) &client_addr, &tamano_client_addr);

		if(otroSock == -1){
			perror("accept()");
			exit(EXIT_FAILURE);
		}

		int pid = fork();

		if(pid == -1){
			perror("fork()");
			exit(EXIT_FAILURE);
		}

		if(pid == 0){
		
			//Creamos la cadena donde guardaremos la peticion del cliente
			char * cadena = (char*) malloc(sizeof(char)*80);

			//Recibimos la peticion del cliente
			if(recv(otroSock, cadena, 80, 0) == -1){
				perror("recv()");
				exit(EXIT_FAILURE);
			}	
		
			//Creamos la cadena donde ira la respuesta del servidor una unidad mas grande que la cadena original para poner al fina el caracter '\0'
			char cadena_respuesta [strlen(cadena) +1];
			int i;
		
			//Recorremos la cadena dada por el cliente
			for(i = 0; i <= strlen(cadena); i++){

				//Si la letra es minuscula usamos la funcion toupper para pasar a mayuscula 
				if(cadena[i] >= 'a' && cadena[i] <= 'z'){
					cadena_respuesta[i] = toupper(cadena[i]);
				}

				//Si la letra es mayuscula con la funcion tolower pasamos la letra a minuscula
				else if(cadena[i] >= 'A' && cadena[i] <= 'Z'){
					cadena_respuesta[i] = tolower(cadena[i]);
				}

				//Si el caracter no es una letra se deja tal cual esta
				else{
					cadena_respuesta[i] = cadena[i];
				}
			}
		
			//Le anhadimos el caracter terminador de cadena al final
			cadena_respuesta[i] = '\0';	 
		
			//Enviamos la cadena al cliente
			if(send(otroSock, cadena_respuesta, strlen(cadena_respuesta) +1, 0) == -1){
				perror("send()");
				exit(EXIT_FAILURE);
		
			}
			//Notificamos el cierre del socket hijo y lo cerramos tanto para lectura como escritura
			shutdown(otroSock, SHUT_RDWR);
			
			//Hacemos un ultimo recv para asegurarnos que se recibe todo y cerramos el socket
			if(recv(otroSock, cadena, 80, 0) == 0){
			//Liberamos la memoria reservada de la cadena
				free(cadena);
				close(otroSock);
			}
			exit(0);
			
		}
	}

	return 0;
}

void signal_handler(int signal){
	//Si la senhal recibida es SIGINT (CTRL + C) entonces se ejecutan las siguientes lineas
	if(signal == SIGINT){
		//Creamos una cadena para poder recibir los ultimos bytes enviados y notificamos el cierre del socket del hijo tanto para lectura como escritura
		char cadena[80] = "";
		shutdown(otroSock, SHUT_RDWR);

		//Si el recv es igual a 0 se cierra el socket del hijo
		if(recv(otroSock, cadena, 80,0) == 0){
			close(otroSock);	
		}
		
		//Notificamos el cierre del socket del padre tanto para lectura como escritura y cerramos el socket
		shutdown(socketfd, SHUT_RDWR);
		close(socketfd);

	}

	exit(0);
		
}









	 
		
