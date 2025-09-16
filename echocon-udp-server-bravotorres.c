//Practica Tema 5: Bravo Torres, Elisa

#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<ctype.h>


int main(int argc, char ** argv){

	//Nos aseguramos que no haya mas de tres argumentos: Nombre de la funcion, -p y numero de puerto.
	if(argc > 3){
		fprintf(stderr,"La llamada debe ser de la forma: ./echocon-udp-servidor-bravotorres [-p puerto-servidor]");
		exit(EXIT_FAILURE);
	}

	unsigned short puerto;

	//Si el usuario indica el puerto, lo guardamos
	if(argc ==3 && strcmp(argv[1], "-p") == 0){
		sscanf(argv[2],"%hu",&puerto);
		puerto = htons(puerto);
	}
	//Si no se indica puerto, ponemos com puerto el numero 5
	else if (argc == 1){
		puerto =htons(5);
	}

	//Si el usuario no indica bien el puerto le recordamos la llamada
	else{
		fprintf(stderr, "Los parametros son: ./echocon-udp-server-bravotorres [-p puerto_servidor]");
		exit(EXIT_FAILURE);
	}
	
	//Creamos el socket del servidor
	int socketfd = socket(AF_INET, SOCK_DGRAM,0);
	if(socketfd == -1){
		perror("socket()");
		exit(EXIT_FAILURE);
	}

	//Enlazamos el socket con una ip y puerto locales

	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = puerto;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(socketfd,(struct sockaddr *) &my_addr, sizeof(my_addr)) == -1){
		perror("bind()");
		exit(EXIT_FAILURE);
	}

	//Iniciamos el bucle de espera de peticiones
	while(1){
		//Creamos la cadena donde guardaremos la peticion del cliente
		char * cadena = (char*) malloc(sizeof(char)*80);

		//Creamos la estructura donde guardaremos la IP del cliente
		struct sockaddr_in client_addr;

		socklen_t tamano_client_addr = sizeof(client_addr);

		//Recibimos la peticion del cliente
		if(recvfrom(socketfd, cadena,80, 0, (struct sockaddr*) &client_addr, &tamano_client_addr) == -1){
			perror("recvfrom()");
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
		if(sendto(socketfd,cadena_respuesta,strlen(cadena_respuesta) +1, 0, (struct sockaddr*) &client_addr, tamano_client_addr) == -1){
			perror("sendto()");
			exit(EXIT_FAILURE);
		
		}
		
		//Liberamos la memoria reservada de la cadena
		free(cadena);
	}

	return 0;
}
		
