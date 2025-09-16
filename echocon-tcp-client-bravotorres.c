//Practica Tema 6: Bravo Torres, Elisa

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>
#include<sys/types.h>



int main(int argc, char** argv){

	//Declaramos la estructura donde guardamos la IP del servidor

	struct in_addr* ip_server =(struct in_addr*) malloc(sizeof(struct in_addr));
        //Nos aseguramos que al menos la funcion tenga 2 parametros: direccion IP y la cadena

	if(argc != 3 && argc != 5) {
	 	fprintf(stderr, "Los parametros son: ip_servidor [-p puerto] cadena\n");
		exit(EXIT_FAILURE);
	}
	
	char * cadena = argv[argc-1];

	//Convertimos la cadena con la IP a un entero de 32 bit.
	if(inet_aton(argv[1],ip_server) == 0){
		perror("inet_aton()");
		exit(EXIT_FAILURE);
	}

	//Guardamos el puerto
	unsigned short puerto;

	//Si el usuario indica el puerto lo convertimos a network byte order
	if(strcmp(argv[2],"-p") == 0 && argc == 5){
		sscanf(argv[3],"%hu",&puerto);
		puerto = htons(puerto);

		//Si el puerto no es el 5 error
		if(puerto != htons(5)){
			fprintf(stderr,"El puerto debe ser el 5\n");
			exit(EXIT_FAILURE);
		}

	}

	//Si no le indicamos nosotros que  el puerto es el 5
	else if (argc == 3){
		 puerto = htons(5);
	}
	//Si el usuario no indica bien el puerto le recordamos la sintaxis de la llamada
	else{
		fprintf(stderr,"Los parametros son: ip_servidor [-p puerto] cadena\n");
		exit(EXIT_FAILURE);
	}

	
	//Nos aseguramos que la cadena tiene como maximo 80 caracteres
	if(strlen(argv[argc-1]) > 80){
		fprintf(stderr,"La cadena no debe superar los 80 caracteres");
		exit(EXIT_FAILURE);
	}
	
	
	//Creamos el socket del cliente
	//El flag AF_INET indica que es una IP version 4
	//Con SOCK_STREAM  indicamos que sera un socket TCP
	int sockfd = socket(AF_INET, SOCK_STREAM,0);
	if(sockfd == -1){
		perror("socket()");
		exit(EXIT_FAILURE);	
	}


	//Estructura sockaddr_in para guardar la IP y puerto locales
	//El flag INADDR_ANY deja que el sistema operativo rellene este campo con la IP del cliente
	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = 0;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	
	//Enlazamos la IP y puerto locales al socket con bind()
	if(bind(sockfd, (struct sockaddr*) &my_addr, sizeof(my_addr)) == -1){
		perror("bind()");
		exit(EXIT_FAILURE);
	}

	//Guardamos la informacion sobre la IP y puerto del servidor
	struct sockaddr_in servidor_addr;
	servidor_addr.sin_family = AF_INET;
	servidor_addr.sin_port = puerto;
	servidor_addr.sin_addr.s_addr = ip_server -> s_addr;

	//Guardamos en una variable el tamanho del servidor_addr
	socklen_t tamano_servidor_addr = sizeof(servidor_addr);


	//Conectamos el socket con el servidor indicandole el socket del cliente, la direccion IP y puerto del servidor y el tamanho de la estructura que lo contiente 
	if(connect(sockfd, (struct sockaddr*) &servidor_addr, tamano_servidor_addr) == -1){
		perror("connect()");
		exit(EXIT_FAILURE);
	}

	//Enviamos la cadena al servidor indicandole el socket, la cadena que se envia, los datos que se envian.
	if(send(sockfd, cadena, strlen(cadena), 0) == -1){
		perror("send()");
		exit(EXIT_FAILURE);
	}	 
	

	//Creamos una cadena vacia donde ira la cadena respuesta
	char cadena_serv [81] = "";
	
	//Recibimos la cadena resultado y la metemos en la variable cadena_serv
	if(recv(sockfd, cadena_serv, strlen(cadena) +1,  0) == -1){
		perror("recv()");
		exit(EXIT_FAILURE);
	}
 
	//Imprimimos la cadena resultado
	printf("%s\n",cadena_serv);
	
	//Notificamos al servidor que queremos cerrar la conexion pasando como parametro el descriptor del socket cliente y la opcion SHUT_RDWR  que especifica que desactivamos la emision y la recepcion de mensajes
	shutdown(sockfd, SHUT_RDWR);

	//Cerramos el socket
	close(sockfd);
	
	//Liberamos el espacio que hemos reservado para ip_server
	free(ip_server);
	return 0;
}

