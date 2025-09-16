//Practica Tema 7: Bravo Torres, Elisa

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>
#include<sys/types.h>
#include<netdb.h>



int main(int argc, char** argv){

	//Declaramos la estructura donde guardamos la IP del servidor
	struct in_addr* ip_server =(struct in_addr*) malloc(sizeof(struct in_addr));
	int verboso = 0;       

	//Si no hay la cantidad de parametros justos imprimimos mensaje de error
	if(argc < 4 || argc > 5) {
	 	fprintf(stderr, "Los parametros son: ip_servidor {-r | -w} archivo [-v]\n"); 
		exit(EXIT_FAILURE);
	}
	

	//Convertimos la cadena con la IP a un entero de 32 bit.
	if(inet_aton(argv[1],ip_server) == 0){
		perror("inet_aton()");
		exit(EXIT_FAILURE);
	}

	//Estructura servent donde guardaremos la informacion sobre el puerto asociado al servicio tftp
	struct servent * puertoTftp = getservbyname("tftp", "udp");

	
	//Creamos el socket del cliente
	//AF_INET: indicamos que es un socket ipV4
	//SOCK_DGRAM: indicamos que es un socket UDP
	int sockfd = socket(AF_INET, SOCK_DGRAM,0);
	if(sockfd == -1){
		perror("socket()");
		exit(EXIT_FAILURE);	
	}

	//Enlazamos el socket con una IP y puerto locales usando bind()
	//Estructura sockaddr_in para guardar la IP y puerto locales
	//INADDR_ANY: pone la ip de nuestra maquina
	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = 0;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	
	if(bind(sockfd, (struct sockaddr*) &my_addr, sizeof(my_addr)) == -1){
		perror("bind()");
		exit(EXIT_FAILURE);
	}

	//Guardamos la informacion sobre la IP y puerto del servidor
	//sin_port: guardamos el puerto que hemos conseguido con la funcion getservbyname, almacenada en s_port del struct servent
	//sin_addr.s_addr: almacenamos la ip del servidor que hemos almacenado en el struct in_addr
	struct sockaddr_in servidor_addr;
	servidor_addr.sin_family = AF_INET;
	servidor_addr.sin_port = puertoTftp -> s_port;
	servidor_addr.sin_addr.s_addr = ip_server -> s_addr;

	//Guardamos en una variable el tamanho del servidor_addr
	socklen_t tamano_servidor_addr = sizeof(servidor_addr);

	//Activamos el modo verboso si el usuario ha incluido "-v" como ultimo parametro
	if(argc == 5){
		if(strcmp(argv[4], "-v") == 0){

			verboso = 1;
		}

		//Si lo ha escrito mal, mostramos mensaje de error y finalizamos
		else{
			fprintf(stderr, "Los parametros son: ip_servidor {-r|-w} nombre_archivo [-v]\n");
			close(sockfd);
			exit(EXIT_FAILURE);
		}
	}

	//Creamos el buffer donde almacenaremos tanto los datagramas enviados como los recibidos
	char buffer[516] = "";
	int tamanoDatagrama = 2;
	buffer[0] = 0;
	
	//Modo lectura
	if(strcmp(argv[2],"-r") == 0){

		//ABRIMOS EL FICHERO DONDE GUARDAREMOS LOS DATOS
		FILE * fichero = fopen(argv[3], "w");
		if(fichero== NULL){
			perror("fopen()");
			close(sockfd);
			exit(EXIT_FAILURE);
		}

		//Creamos el mensaje RRQ = opcode(01) + nombre_fichero\0 + mode\0
		buffer[1] = 1;
		tamanoDatagrama += (strlen(argv[3])+ 1);
		strcpy(&buffer[2], argv[3]);
		strcpy(&buffer[tamanoDatagrama], "octet\0");
		tamanoDatagrama += 6;
		
		int bloqueCont = 0;

		//Enviamos la peticion RRQ al servidor
		if(sendto(sockfd, buffer, tamanoDatagrama, 0, (struct sockaddr*) &servidor_addr, tamano_servidor_addr) == -1){
			perror("sendto()");
			fclose(fichero);
			close(sockfd);
			exit(EXIT_FAILURE);
		}
  
		if(verboso == 1){
			printf("Enviada solicitud de lectura de %s a servidor tftp (10.0.25.250)\n",argv[3]);
		} 
 		
		
		//Recibimos la respuesta del servidor
		int bytesRecibidos = recvfrom(sockfd, buffer, 516,0,(struct sockaddr*) &servidor_addr, &tamano_servidor_addr);
		

		//BUCLE CON RECVFROM SI RECVFROM - 4 ES MENOR DE 512 SE ENVIA EL ULTIMO ACK Y SE CIERRA EL FICHERO	
		while(bytesRecibidos - 4 == 512){	
			
			//Comprobamos que no sea un mensaje de error mirando si el opcode es 05, en caso afirmativo imprimimos el mensaje de error
			if(buffer[1] == '5'){
				fprintf(stderr, "%c, %c, %s", buffer[2], buffer[3], &buffer[4]);
				fclose(fichero);
				close(sockfd);
				exit(EXIT_FAILURE);
			}

  			bloqueCont += 1;

			//Comparamos el bloque que nos devuelve el servidor en el paquete Data con nuestro contador local para verificar que los mensajes llegan en orden
			int bloqueServidor = ((unsigned char) buffer[2]) * 256;
			bloqueServidor += (unsigned char)buffer[3];

			//Si son distintos, imprimimos mensaje de error y cerramos fichero y socket
			if(bloqueCont != bloqueServidor){
				fprintf(stderr, "El numero de bloque esperado era %d pero fue %d\n", bloqueCont, bloqueServidor);
				fclose(fichero);
				close(sockfd);
				exit(EXIT_FAILURE);
			}

			printf("Recibido bloque del servidor tftp\n");  
			printf("Es el bloque %d\n", bloqueCont);
	
			//Escribimos los datos recibidos del servidor en el fichero
			fwrite(&buffer[4],sizeof(char), 512,fichero);


			if(verboso == 1){
				printf("Enviamos el ACK del bloque %d\n", bloqueCont);
			}

			//CAMBIO OPCODE A 04 PARA ENVIAR ACK
			buffer[1] = 4;

			//ENVIAMOS ACK
			if(sendto(sockfd, buffer, 4, 0, (struct sockaddr*) &servidor_addr, tamano_servidor_addr) == -1){
				perror("sendto()");
				fclose(fichero);
				close(sockfd);
				exit(EXIT_FAILURE);
			}

			//Recibimos otro datagrama del servidor
			bytesRecibidos = recvfrom(sockfd, buffer, 516, 0, (struct sockaddr*) &servidor_addr, &tamano_servidor_addr);

		}

		//SI RECIBEN MENOS DE 512 BYTES DE DATOS
		//Incrementamos nuestro contador local de bloques
		bloqueCont += 1;

		//Vemos si recvfrom ha dado error
		if(bytesRecibidos == -1){
			perror("recvfrom()");
			fclose(fichero);
			close(sockfd);
			exit(EXIT_FAILURE);
		}

		//Fijarse en el opcode porque si es 05 es un error
		if(buffer[1] == '5'){
			fprintf(stderr, "%c, %c, %s\n", buffer[2], buffer[3], &buffer[4]);
			fclose(fichero);
			close(sockfd);
			exit(EXIT_FAILURE);
		}
		
		//Volvemos a comprobar el numero de bloque que nos manda el servidor con nuestro contador local
		int bloqueServidor = ((unsigned char) buffer[2]) * 256;
		bloqueServidor +=(unsigned char) buffer[3];

		if(bloqueCont != bloqueServidor){
			fclose(fichero);
			close(sockfd);
			exit(EXIT_FAILURE);
		} 

		printf("Recibido bloque del servidor tftp\n");
		printf("Es el bloque numero %d\n", bloqueCont);

		//Escribimos en el fichero
		fwrite(&buffer[4], sizeof(char), bytesRecibidos-4, fichero);

		if(verboso == 1){
			printf("Enviamos el ACK del bloque %d\n", bloqueCont);
		}

		//MANDAMOS EL ULTIMO ACK OPCODE (04) Y EL NUMERO DE BLOQUE QUE ACABAMOS DE RECIBIR
		buffer[1] = 4;
		
		if(sendto(sockfd, buffer, 4, 0, (struct sockaddr*) &servidor_addr, tamano_servidor_addr) == -1){
			perror("sendto()");
			fclose(fichero);
			close(sockfd);
			exit(EXIT_FAILURE);
		}

		printf("El bloque %d era el ultimo: cerrando fichero\n", bloqueCont);

		//SE CIERRA EL FICHERO Y EL SOCKET	
		fclose(fichero);
		close(sockfd);

	}

	//MODO ESCRITURA
	else
	if(strcmp(argv[2], "-w") == 0){

		//Abrimos un fichero local en modo lectura
		FILE * fichero = fopen(argv[3], "r");
		if(fichero == NULL){
			perror("fopen()");
			exit(EXIT_FAILURE);
		}
         	
		//Creamos el mensaje WRQ: OPCODE (02) + NOMBRE_ARCHIVO + \0 + OCTET + \0
		buffer[1] = 2;
		tamanoDatagrama += (strlen(argv[3]) + 1);
		strcpy(&buffer[2], argv[3]);
		strcpy(&buffer[tamanoDatagrama], "octet\0");
		tamanoDatagrama += 6;
		
		int bloqueCont = 0;

		//Enviamos la cadena al servidor
		if(sendto(sockfd, buffer, tamanoDatagrama, 0, (struct sockaddr*) &servidor_addr, tamano_servidor_addr) == -1){
			perror("sendto()");
			fclose(fichero);
			close(sockfd);
			exit(EXIT_FAILURE);
		} 
		  
		if(verboso == 1){
			printf("Enviada la solicitud de escritura de %s a servidor tftp (10.0.25.250)\n", argv[3]);
		}

		//RECIBIMOS EL ACK DEL SERVIDOR
		if(recvfrom(sockfd, buffer, 516, 0, (struct sockaddr*) &servidor_addr, &tamano_servidor_addr) == -1){
			perror("recvfrom()");
			fclose(fichero);
			close(sockfd);
			exit(EXIT_FAILURE);
		}

		//Comprobamos que no sea un mensaje de error, observando el opcode (05 si es error)
		if(buffer[1] == '5'){
			fprintf(stderr, "%c, %c, %s\n", buffer[2], buffer[3], &buffer[4]);
			fclose(fichero);
			close(sockfd);
			exit(EXIT_FAILURE);
		}
		
		//Comprobamos el numero de bloque que nos ha llegado en el ACK del servidor con nuestro contador local
		int bloqueServidor = ((unsigned char) buffer[2]) * 256;
		bloqueServidor +=(unsigned char) buffer[3];

		if(bloqueCont != bloqueServidor){
			fprintf(stderr, "El ACK esperado era %d pero fue %d\n", bloqueCont, bloqueServidor);
			fclose(fichero);
			exit(EXIT_FAILURE);
		}

		
		printf("Recibido ACK del bloque %d\n", bloqueCont);
		
		//Incrementamos el contador local
		bloqueCont += 1;
		
		//Vamos leyendo el fichero en bloques de 512 bytes, lo maximo del campo de datos del mensaje de tipo DATA
		int bytesEnviados = 0;
		bytesEnviados = fread(&buffer[4], sizeof(char), 512, fichero);
		
		//SI SE MANDAN 512 BYTES EN EL CAMPO DATA
		while(bytesEnviados == 512){
			//Cambiamos el opcode a 03
			buffer[1] = 3;
			//Calculamos el numero de bloque
			buffer[2] = bloqueCont / 256;
			buffer[3] = bloqueCont % 256;

		//ENVIAMOS UN PAQUETE DE TIPO DATA AL SERVIDOR
		       if(sendto(sockfd, buffer,bytesEnviados + 4 , 0, (struct sockaddr*) &servidor_addr, tamano_servidor_addr) == -1){
				perror("sendto()");
				fclose(fichero);
				close(sockfd);
				exit(EXIT_FAILURE);
			}

			if(verboso == 1){
				printf("Enviado bloque de datos %d\n", bloqueCont);
			}

		//RECIBIMOS LA RESPUESTA DEL SERVIDOR
			if(recvfrom(sockfd, buffer, 516, 0, (struct sockaddr*) &servidor_addr, &tamano_servidor_addr)==-1){
				perror("recvfrom()");
				fclose(fichero);
				close(sockfd);
				exit(EXIT_FAILURE);
			}
			
			//Comprobamos que la respuesta es un ACK y no un error
			if(buffer[1] == '5'){
				fprintf(stderr, "%c, %c, %s\n", buffer[2], buffer[3], &buffer[4]);
				fclose(fichero);
				close(sockfd);
				exit(EXIT_FAILURE);
			}
	
			//Comprobamos que el bloque del servidor es el mismo que el de nuestro contador local
			bloqueServidor = ((unsigned char) buffer[2]) * 256;
			bloqueServidor += (unsigned char)buffer[3];

			if(bloqueCont != bloqueServidor){
				fprintf(stderr, "El ACK esperado era %d pero fue %d\n", bloqueCont, bloqueServidor);
				fclose(fichero);
				exit(EXIT_FAILURE);
			}

	
			printf("Recibido ACK del bloque %d\n", bloqueCont);
		
			//Incrementamos el contador de bloques
			bloqueCont += 1;
			//Leemos otros 512 bytes del fichero
			bytesEnviados = fread(&buffer[4], sizeof(char), 512, fichero);

		}

		//SI LLEGAMOS AL FINAL DEL FICHERO
		
		
		//MANDAMOS EL ULTIMO BLOQUE DATA CAMBIANDO EL OPCODE (03) Y CALCULANDO EL NUMERO DE BLOQUE

		buffer[1] = 3;
		buffer[2] = bloqueCont / 256;
		buffer[3] = bloqueCont % 256;

		bytesEnviados += 4;
		if(sendto(sockfd, buffer, bytesEnviados, 0,(struct sockaddr*) &servidor_addr, tamano_servidor_addr) == -1){
			perror("sendto()");
			fclose(fichero);
			close(sockfd);
			exit(EXIT_FAILURE);
		}
	
		if(verboso == 1){
			printf("Enviado el bloque de datos %d: Es el ultimo\n", bloqueCont);
		}

		//RECIBIMOS EL ULTIMO ACK
		if(recvfrom(sockfd, buffer, 516, 0, (struct sockaddr*) &servidor_addr, &tamano_servidor_addr) == -1){
			perror("sendto()");
			fclose(fichero);
			close(sockfd);
			exit(EXIT_FAILURE);
		}

		//Comprobamos que es un ACK y no un error	
		if(buffer[1] == '5'){
			fprintf(stderr, "%c, %c, %s\n", buffer[2], buffer[3], &buffer[4]);	
			fclose(fichero);
			close(sockfd);
			exit(EXIT_FAILURE);
		}

		//Comprobamos que el bloque que nos envia el servidor es el mismo que el de nuestro contador local	
		bloqueServidor = ((unsigned char) buffer[2]) * 256;
		bloqueServidor += (unsigned char) buffer[3];

		if(bloqueCont != bloqueServidor){
			fprintf(stderr, "El ACK esperado era %d pero fue %d\n", bloqueCont, bloqueServidor);
			fclose(fichero);
			close(sockfd);
			exit(EXIT_FAILURE);
		}

		if(verboso == 1){
			printf("Recibido el utlimo ACK del servidor\n");
			printf("Cerrando fichero\n");
		}

		fclose(fichero);
		close(sockfd);
	}
	//Si usuario escribe un modo distinto de -r o -w imprimimos mensaje de error y cerramos socket
	else {
		fprintf(stderr, "Los parametros son: ip_servidor {-r | -w} nombre_fichero [-v]\n");
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	
	//Liberamos el espacio que hemos reservado para ip_server
	free(ip_server);
	return 0;
}

