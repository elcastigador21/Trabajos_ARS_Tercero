//Practica Tema 8: Bravo Torres, Elisa

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>
#include<sys/types.h>
#include "ip-icmp-ping.h"

//Declaramos como variables globales las variables que guardaran la peticion echo y la respuesta. 
EchoRequest echoRq;
EchoReply echoRp;

void erroresPing();
unsigned short calculoChecksum();
int main(int argc, char** argv){

	struct in_addr* ip_dst = (struct in_addr*) malloc(sizeof(struct in_addr));	
	int verboso = 0;
	
	//Si el usuario introduce argumentos de mas o de menos le mostramos un mensaje de error indicando los parametros correctos.
	if(argc < 2 || argc > 3){
		fprintf(stderr, "Los parametros son: direccion_ip [-v]\n");
		exit(EXIT_FAILURE);
	}

	//Comprobamos si el usuario ha activado correctamente el modo verboso
	if(argc == 3){
		if(strcmp(argv[2], "-v") == 0){
			verboso = 1;
		}
		else{
			fprintf(stderr,"Los parametros son: direccion_ip [-v]\n");
			exit(EXIT_FAILURE);
		}
	}

	if(inet_aton(argv[1], ip_dst) == 0){
		perror("inet_aton()");
		exit(EXIT_FAILURE);
	}
	
	//Creamos el socket indicando en este caso que va a ser un socket de tipo RAW para acceder directamente a la capa IP
	//IPPROTO_ICMP: indica que el protocolo es ICMP
	int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if(sockfd < 0){
		perror("sockfd()");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = 0;
	my_addr.sin_addr.s_addr = INADDR_ANY;

	//Enlazamos nuestra direccion IP local con el socket
	if(bind(sockfd, (struct sockaddr*) &my_addr, sizeof(my_addr)) < 0){
		perror("bind()");
		exit(EXIT_FAILURE);
	}
	//Guardamos la direccion ip del destino indicando que es una ipv4 (AF_INET) y el puerto 0, dado que icmp no utiliza puertos
	struct sockaddr_in dst_addr;
	dst_addr.sin_family = AF_INET;
	dst_addr.sin_port = 0;
	dst_addr.sin_addr.s_addr = ip_dst -> s_addr;

	socklen_t tamano_dst_addr = sizeof(dst_addr);
	
	//CONSTRUIMOS EL DATAGRAMA
	//TYPE: 8 porque es una Request
	//CODE: 0
	//PID: usamos la funcion getpid para conseguir el PID del proceso
	//Sequence: numero de secuencia inicializado a 0
	//Payload: mensaje a enviar, con una logitud maxima de 64 bytes.

	echoRq.icmpHdr.type = 8;
	echoRq.icmpHdr.code = 0;
	echoRq.icmpHdr.checksum = 0;
	echoRq.pid = (unsigned short) getpid();
	echoRq.sequence = 0;
	strcpy(echoRq.payload, "Hola me llamo pepe");

	//CALCULO CHECKSUM
	//FUNCION QUE CALCULA CHECKSUM Y GUARDA EL RESULTADO EN EL APARTADO CHECKSUM DE LA CABECERA ICMP
	echoRq.icmpHdr.checksum = calculoChecksum();

	//COMPROBACION DE QUE EL CHECKSUM ESTA BIEN CALCULADO
	//printf("%d\n", calculoChecksum());

	if(verboso == 1){
		printf("-> Generando cabecera ICMP...\n");
		printf("-> Type: %d\n", echoRq.icmpHdr.type);
		printf("-> Code: %d\n", echoRq.icmpHdr.code);
		printf("-> PID: %d\n", echoRq.pid);
		printf("-> Sequence number: %d\n", echoRq.sequence);
		printf("-> Cadena a enviar: %s\n", echoRq.payload);
		printf("-> Checksum: %d\n", echoRq.icmpHdr.checksum);
		printf("-> Tamanho del datagrama: %d\n", (int)sizeof(echoRq));
	}

	printf("Mensaje enviado al Host: %s\n", argv[1]);

	//Enviamos el datagrama ICMP al destino, haciendo un cast de la variable echoRq a char * para poder enviar el datagrama
	
	if(sendto(sockfd,(char *) &echoRq, sizeof(EchoRequest), 0, (struct sockaddr*) &dst_addr, tamano_dst_addr) < 0){
		perror("sendto()");
		exit(EXIT_FAILURE);
	}
	
	//Recibimos la respuesta ICMP, la cual tiene 20 bytes mas por la cabezera IP que contiene
	if(recvfrom(sockfd, (char *) &echoRp, sizeof(EchoReply), 0, (struct sockaddr*) &dst_addr, &tamano_dst_addr) < 0 ){
		perror("recvfrom()");
		exit(EXIT_FAILURE);
	
	}

	
	//FUNCION ERRORES: si el tipo del datagrama no es 0, es poque se ha producido un error
	if(echoRp.icmpMsg.icmpHdr.type != 0){
		erroresPing();
	}
	else{
		if(verboso == 1){
			printf("-> Tamanho de la respuesta %d\n", (int)sizeof(echoRp));
			printf("-> Cadena Recibida: %s\n", echoRp.icmpMsg.payload);
			printf("-> PID: %d\n", echoRp.icmpMsg.pid);
			printf("-> TTL: %d\n", echoRp.ipHdr.TTL);
		}
		printf("Respuesta correcta\n");

	}
	//Imprimimos el tipo y el codigo del mensaje
	printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);

	close(sockfd);
	
	return 0;


}



unsigned short calculoChecksum(){
	//Acumulador almacenara las sumas parciales
	unsigned int acumulador = 0;
	//Puntero al datagrama ICMP que se quiere enviar y que recorrera las medias palabras
	unsigned short int *puntero = (unsigned short int*) &echoRq;
	int i;
	//tamanoDatagrama indica el numero de medias palabras del datagrama
	int tamanoDatagrama = sizeof(echoRq)/2;
	
	//Recorremos las medias palabras del datagrama haciendo las sumas parciales
	for(i=0; i < tamanoDatagrama - 1 ; i++){
		acumulador += (unsigned int) *puntero;
		puntero++;
	}
	//Para que las sumas parciales sean correctas, sumamos la parte alta del acumulador con la parte baja y repetimos este proceso otra vez por is hubiera acarreo
	acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);
	acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);

	//Al resultado se le hace un not logico para obtener el complemento a 1 y se devuelve el resultado
	acumulador = ~(acumulador & 0x0000ffff);
	return acumulador;

}

//Funcion que imprime los mensajes de error segun el tipo y el codigo del EchoReplay
void erroresPing(){
	switch(echoRp.icmpMsg.icmpHdr.type){
		//Caso Destination Unreachable
		case 3:
			if(echoRp.icmpMsg.icmpHdr.code == 0){
				fprintf(stderr,"Destination Unreachable: Net Unreachable\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);
			}
			else if(echoRp.icmpMsg.icmpHdr.code == 1){
				fprintf(stderr, "Destination Unreachable: Host Unreachable\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);
			}
			else if(echoRp.icmpMsg.icmpHdr.code == 2){
				fprintf(stderr, "Destination Unreachable: Protocol Unreachable\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);
			}
			else if(echoRp.icmpMsg.icmpHdr.code == 3){
				fprintf(stderr, "Destination Unreachable: Port Unreachable\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);
			}
			else if(echoRp.icmpMsg.icmpHdr.code == 4){
				fprintf(stderr, "Destination Unreachable: Fragmentation Needed\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);
			}
			else if(echoRp.icmpMsg.icmpHdr.code == 5){
				fprintf(stderr, "Destination Unreachable: Source Route Failed\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);
			}
			else if(echoRp.icmpMsg.icmpHdr.code == 6){
				fprintf(stderr, "Destination Unreachable: Destination Network Unknown\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);

			}
			else if(echoRp.icmpMsg.icmpHdr.code == 7){
				fprintf(stderr, "Destination Unreachable: Destination Host Unknown\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);
			}
			else if(echoRp.icmpMsg.icmpHdr.code == 8){
				fprintf(stderr, "Destination Unreachable: Source Host Isolated\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);
			}
			else if(echoRp.icmpMsg.icmpHdr.code == 11){
				fprintf(stderr, "Destination Unreachable: Destination Network Unreachable for Type of Service\n");

				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);

				exit(EXIT_FAILURE);
			}         
			else if(echoRp.icmpMsg.icmpHdr.code == 12){
				fprintf(stderr, "Destination Unreachable: Destination Host Unreachable for Type of Service\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);

				exit(EXIT_FAILURE);
			}
			else if(echoRp.icmpMsg.icmpHdr.code == 13){
				fprintf(stderr, "Destination Unreachable: Communication Administratively Prohibited\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);
			}
			else if(echoRp.icmpMsg.icmpHdr.code == 14){
				fprintf(stderr, "Destination Unreachable: Host Procedence Violation\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);
			}
			else if(echoRp.icmpMsg.icmpHdr.code == 15){
				fprintf(stderr, "Destination Unreachable: Precedence Cutoff in Effect\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);
			}
			break;
		//Caso Redirect
		case 5:
			if(echoRp.icmpMsg.icmpHdr.code == 1){
				fprintf(stderr, "Redirect: redirect for destination host\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);
			}
			else if(echoRp.icmpMsg.icmpHdr.code == 3){
				fprintf(stderr, "Redirect: redirect for destination host based on type of service\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);
			}
			break;
		//Caso Time Exeeded
		case 11:
			if(echoRp.icmpMsg.icmpHdr.code == 0){
				fprintf(stderr, "Time exceeded: Time-to-Live Exceeded in Transit\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);
			}
			else if(echoRp.icmpMsg.icmpHdr.code == 1){
				fprintf(stderr, "Time exceeded: Fragment Reassembly Time Exceeded\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);
			}
			break;
		//Parameter problem
		case 12:
			if(echoRp.icmpMsg.icmpHdr.code == 0){
				fprintf(stderr, "Parameter Problem: Pointer indicates the error\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
			
				exit(EXIT_FAILURE);
			}
			else if(echoRp.icmpMsg.icmpHdr.code == 1){
				fprintf(stderr, "Parameter Problem: Missing a Required Option\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);
			}
			else if(echoRp.icmpMsg.icmpHdr.code == 2){
				fprintf(stderr, "Parameter Problem: Bad Length\n");
				printf("(Type: %d, Code: %d)\n", echoRp.icmpMsg.icmpHdr.type, echoRp.icmpMsg.icmpHdr.code);
				exit(EXIT_FAILURE);
			}
			break;
	}

			
				
				
			
}
