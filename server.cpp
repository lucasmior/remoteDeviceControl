#include <iostream>		// std
#include <cstring>		// strcpy
#include <vector>		// vector


#include <stdio.h>		/* for printf() and fprintf() */
#include <sys/socket.h>	/* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>	/* for sockaddr_in and inet_addr() */
#include <stdlib.h>		/* for atoi() and exit() */
#include <string.h>		/* for memset() */
#include <unistd.h>		/* for close() */


#include <stdlib.h>		// malloc
#include <pthread.h>	// pthread
#include <unistd.h>		// sleep

using namespace std;

#define ECHOMAX 255		/* Longest string to echo */
#define MAXPENDING 5    /* Numero maximo de conexoes que podem ficar esperando serem aceitas */
#define RCVBUFSIZE 32   /* Tamanho do buffer de recebimento */

#define ERROR -1

enum TCP_CMD
{
	NAME,
	READ,
	WRITE,
	SHUTDOWN,
	HOSTS,
	TCP_SIZE,
};

enum UDP_CMD
{
	START,
	HEARTBEAT,
	UDP_SIZE,
};

struct neighbor
{
	char ip[16];
	char name[255];
}neighbor;

struct sharedData
{
	int udpPort;
	int tcpPort;
	char ip[16];
	char broadcast[16];
	char name[255];
	pthread_mutex_t mutex;
	std::vector<struct neighbor> neigh;

}sharedData;
////////////////////////////////////////
/// @brief 
///
///	@param
///	@param
///	@param
////////////////////////////////////////
void sendUdpCmd( char* servIP, int udpPort, char* echoString )
{
	int sock;							/* Socket descriptor */
	int broadcast = 1;					/* Enable broadcast */
	int echoStringLen;					/* Length of string to echo */
	struct sockaddr_in echoServAddr;	/* Echo server address */
	struct sockaddr_in fromAddr;		/* Source address of echo */

	/* Create a datagram/UDP socket */
	if( ( sock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 )
	{
		perror( "socket" );
		exit( 1 );
	}
	if( ( setsockopt( sock, SOL_SOCKET, SO_BROADCAST,
					  &broadcast, sizeof broadcast ) ) == -1 )
	{
		perror( "setsockopt" );
		exit( 1 );
	}

	/* Construct the server address structure */
	memset( &echoServAddr, 0, sizeof(echoServAddr) );	/* Zero out structure */
	echoServAddr.sin_family = AF_INET;	/* Internet addr family */
	echoServAddr.sin_addr.s_addr = inet_addr(servIP);	/* Server IP address */
	echoServAddr.sin_port = htons(udpPort);	/* Server port */

	if( ( echoStringLen = strlen(echoString) ) > ECHOMAX )
 	{	/* Check input length */
		printf( "Echo word too long" );
		exit( 1 );
	}

	/* Send the string to the server */
	if( sendto( sock, echoString, echoStringLen, 0, 
		        (struct sockaddr *)&echoServAddr, 
		        sizeof (echoServAddr) ) != echoStringLen )
	{
		perror( "sendto" );
		exit( 1 );
	}
	
	close(sock);
}

int getCmd( char* cmd, char* param, char* echoBuffer )
{

	int recvMsgSize = strlen( echoBuffer );
	int flag = 0;
	int tam = 0;
	for( int i = 0; i < recvMsgSize; i++ )
    {
    	if( echoBuffer[i] == ' ' )
    	{
    		cmd[i] = '\0';
    		flag = 1;
    	}
    	else if( flag )
    	{
    		param[tam] = echoBuffer[i];
    		tam++;
    	}
    	else 
    	{
    		cmd[i] = echoBuffer[i];
    	}
    }
    param[tam] = '\0';
    cmd[recvMsgSize] = '\0';

	if( strcmp( cmd, "HEARTBEAT" ) != 0 && strcmp( cmd, "START" ) != 0 )
	{
		cout << "Invalid command!" << endl;
		return 0;
	}
	return 1;		//cmd size
}

void updateServer( char* ip, char* name, struct sharedData* shared )
{
	pthread_mutex_lock( &shared->mutex );
	for( int i = 0; i < shared->neigh.size( ); i++ )
	{
		if( strcmp( shared->neigh.at(i).ip, ip ) == 0 )
		{
			strcpy( shared->neigh.at(i).name, name );
			// TODO update the timestamp
			pthread_mutex_unlock( &shared->mutex );
			return;
		}
	}
	
	struct neighbor	n;
	strcpy( n.name, name );
	strcpy( n.ip, ip );
	shared->neigh.push_back( n );

	pthread_mutex_unlock( &shared->mutex );
	return;
}

void *heartbeat_function( void *arg )
{
	struct sharedData *shared = (struct sharedData*)arg;

	char *servIP;					/* IP address of server */
	unsigned short udpPort;			/* Echo server port */

	servIP 	= shared->broadcast; 	// Broadcast ip, hard-coded value
	udpPort = shared->udpPort;	 	// Server ports, hard-coded value

	char *echoString;				/* String to send to echo server */

	while( 1 )
	{
		pthread_mutex_lock( &shared->mutex );
		echoString = (char*)malloc( 10 + strlen(shared->name) );	/* Third arg: string to echo */
		strcpy( echoString, "HEARTBEAT " );
		strcpy( &echoString[10], shared->name );
		pthread_mutex_unlock( &shared->mutex );

		sendUdpCmd( servIP, udpPort, echoString );
		sleep( 10 );
	}
}

void *update_function( void *arg )
{
	struct sharedData *shared = (struct sharedData*)arg;
	
	int sock;							/* Socket */
	struct sockaddr_in echoServAddr;	/* Local address */
	struct sockaddr_in echoClntAddr;	/* Client address */
	unsigned int cliAddrLen;			/* Length of incoming message */
	char echoBuffer[ECHOMAX];			/* Buffer for echo string */
	unsigned short udpPort;				/* Server port */
	int recvMsgSize;					/* Size of received message */

	udpPort = shared->udpPort;			/* First arg:  local port */

	/* Create socket for sending/receiving datagrams */
	if( ( sock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 )
	{
		perror("socket");
		exit(1);
	}

	/* Construct local address structure */
	memset( &echoServAddr, 0, sizeof (echoServAddr) );	/* Zero out structure */
	echoServAddr.sin_family = AF_INET;					/* Internet address family */
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);	/* Any incoming interface */
	echoServAddr.sin_port = htons(udpPort);		/* Local port */

	/* Bind to the local address */
	if( bind( sock, (struct sockaddr *) &echoServAddr, sizeof (echoServAddr) ) < 0 )
	{
		perror("bind");
		exit(1);
	}
	/* Set the size of the in-out parameter */
	cliAddrLen = sizeof (echoClntAddr);

	while( 1 )
	{

		cout << "Waiting for cmd.. " << endl;
		/* Block until receive message from a client */
		if( ( recvMsgSize = recvfrom( sock, echoBuffer, ECHOMAX, 0,
					    			  (struct sockaddr *)&echoClntAddr,
					    			   &cliAddrLen)) < 0)
	    {
			perror("recvfrom");
			exit(1);
		}

		echoBuffer[recvMsgSize] = '\0';

		printf("Handling client %s\n", inet_ntoa( echoClntAddr.sin_addr ) );
		printf("Message: %s\n", echoBuffer);

		//update neigh
		
		char cmd[recvMsgSize];
		char param[recvMsgSize];
		int valid = getCmd( cmd, param, echoBuffer );
		char answer[256];
		answer[0] = '\0';

		if( valid )	// just valid commands (START, HEARTBEAT)
		{
			struct neighbor n;
			
			strcpy( n.name, param );
			strcpy( n.ip , inet_ntoa( echoClntAddr.sin_addr ) );
			cout << "name is: " << n.name <<   endl
				 << "ip is: " << n.ip << endl;
				
			if( strcmp( cmd, "START" ) == 0 )
			{

				pthread_mutex_lock( &shared->mutex );
				//verify if dont has the same server at the list
				shared->neigh.push_back( n );

				//send a heartbeat go back
				char *echoString = (char*)malloc( 10 + strlen(shared->name) );	/* Third arg: string to echo */
				strcpy( echoString, "HEARTBEAT " );
				strcpy( &echoString[10], shared->name );
				sendUdpCmd( n.ip, udpPort , echoString );

				pthread_mutex_unlock( &shared->mutex );	
			}
			else // HEARTBEAD
			{
				updateServer( n.ip, n.name, shared );	//change to struct
			}
		}
		else
		{
			//nothing
			;;	
		}
	}
}

int isValidCmd( char* cmd, int size )
{
	if( strcmp( cmd, "NAME" ) == 0 || strcmp( cmd, "HOSTS" ) == 0 || 
	    strcmp( cmd, "WRITE" ) == 0 || strcmp( cmd, "READ" ) == 0 ||
	    strcmp( cmd, "SHUTDOWN" ) == 0 )
	{
		return 1;
	}
	return 0;
}

void *recvClientCommands( void *arg )
{
	cout << "start" << endl;
	struct sharedData *shared = (struct sharedData*)arg;
	int servSock;                    	/* Descritor de socket para o servidor */
    int clntSock;                    	/* Descritor de socket para o cliente */
    struct sockaddr_in echoServAddr; 	/* Struct para o endereco local */
    struct sockaddr_in echoClntAddr; 	/* Struct para o endereco do cliente */
    unsigned short tcpPort;     		/* Numero da porta do servidor */
    unsigned int clntLen;            	/* Tamanho da struct de endereco do cliente */
	char echoBuffer[RCVBUFSIZE];        /* Buffer para a string de eco */
    int recvMsgSize;                    /* Tamanho da mensagem recebida */

    tcpPort = shared->tcpPort;  		/* Primeiro argumento:  numero da porta local */

    /* Cria um socket TCP para receber conexoes de clientes */
    if( ( servSock = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP ) ) < 0 )
    {
        perror( "socket" );
        exit( 1 );
    }
      
    /* Constroi uma estrutura para o endereco local */
    memset( &echoServAddr, 0, sizeof(echoServAddr) );  	/* Preenche a struct com zeros */
    echoServAddr.sin_family = AF_INET;                	/* Faminha de enderecos Internet (IP) */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); 	/* Permite receber de qualquer endereco/interface */
    echoServAddr.sin_port = htons(tcpPort);      		/* Porta local */

    /* "Amarra" o endereco local (o qual inclui o numero da porta) para esse servidor */
    if( bind( servSock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr) ) < 0 )
    {
        perror( "bind" );
        exit( 1 );
    }

    /* Faz com que o socket possa "escutar" a rede e receber conexoes de clientes */
    if( listen( servSock, MAXPENDING ) < 0 )
    {
        perror( "listen" );
        exit( 1 );
    }

    int shutdown = 1;
    while( shutdown ) /* Executa para sempre */
    {
    	cout << "recv cmd" << endl;
        /* Configura o tamanho da struct de endereco do cliente */
        clntLen = sizeof(echoClntAddr);

        /* Espera por conexoes de clientes */
        if( ( clntSock = accept( servSock, (struct sockaddr *)&echoClntAddr, 
                                 &clntLen) ) < 0)
        {
            perror( "accept" );
            exit( 1 );
        }

        /* Nesse ponto, um cliente ja conectou!
         * O socket clntSock contem a conexao com esse cliente! */

        printf( "Handling client %s\n", inet_ntoa( echoClntAddr.sin_addr ) );

        /* Essa funcao e' chamada para tratar a conexao com o cliente, a qual e' definida pelo socket clntSock */
    

	    /* Recebe uma mensagem do cliente */
	    if( ( recvMsgSize = recv( clntSock, echoBuffer, RCVBUFSIZE, 0 ) ) < 0 )
	    {
	        perror( "recv" );
	        exit( 1 );
	    }
	    echoBuffer[recvMsgSize] = '\0';
	    cout << "recved" << endl;
	    char cmd[recvMsgSize];
	    char param[recvMsgSize];
	    int flag = 0;
	    int tam = 0;
	    for( int i = 0; i < recvMsgSize; i++ )
	    {
	    	if( echoBuffer[i] == ' ' )
	    	{
	    		cmd[i] = '\0';
	    		flag = 1;
	    	}
	    	else if( flag )
	    	{
	    		param[tam] = echoBuffer[i];
	    		tam++;
	    	}
	    	else 
	    	{
	    		cmd[i] = echoBuffer[i];
	    	}
	    }
	    param[tam] = '\0';
	    cmd[recvMsgSize] = '\0';
	    int valid = isValidCmd( cmd, strlen(cmd) );
	    char answer[256];
	    answer[0] = '\0';

	    cout << "valid? " << valid << endl;
	    if( valid )
	    {
		    //printf("valid cmd: %d\n", valid);
		    //printf("%d: %d\n", strlen(echoBuffer), strlen(cmd));
		    //printf("Have param: %d %s\n", strlen(echoBuffer) > strlen(cmd), param);
	    	cout << "cmd: " << cmd << endl;
	    	//DONE
	    	if( strcmp( cmd, "NAME" ) == 0 )
	    	{
		    	strcpy( answer, "200 OK " );
	    		pthread_mutex_lock( &shared->mutex );
	    		if( strlen(param) )
	    		{
	    			strcpy( shared->name, param );
	    		}
	    		strcpy( &answer[7], shared->name );
	    		pthread_mutex_unlock( &shared->mutex );

	    	}

	    	else if( strcmp( cmd, "WRITE" ) == 0 )
	    	{
	    		//Parse param..
	    		char type[4];
	    		char addr[3];
	    		char value;
	    		cout << strlen(param) << endl;
	    		strncpy( type, param, 4 );
	    		strncpy( addr, &param[4], strlen(param)-8 );
	    		value = param[strlen(param)];

	    		cout << "type: " << type << " addr: " << addr << "value: " << value << endl;
	    		//short addr;

	    	}
	    	else if( strcmp( cmd, "READ" ) == 0 )
	    	{
	    		
	    	}
	    	else if( strcmp( cmd, "SHUTDOWN" ) == 0 )
	    	{
		    	strcpy( answer, "200 OK\0" );
				shutdown = 0;	    		
	    	}
	    	else if( strcmp( cmd, "HOSTS" ) == 0 )
	    	{
	    		pthread_mutex_lock( &shared->mutex );
	    		cout << "Hosts: " << endl;
	    		for( int i = 0; i < shared->neigh.size( ); i++ )
	    		{
	    			cout << "IP: " << shared->neigh.at(i).ip << " name: " << shared->neigh.at(i).name << endl; 
	    		}
	    		pthread_mutex_unlock( &shared->mutex );
	    	}

	    }
	    else
	    {
	    	strcpy( answer, "400 Not Found\0" );
	    	printf( "Invalid Command\n" );
	    }

	    cout << answer << " size: " << strlen(answer) << endl;
	    /* Envia a string recebida de volta ao cliente e recebe outra ate o final da transmissao */
        /* Envia a mensagem de volta ao cliente */
        //if( send( clntSock, echoBuffer, recvMsgSize, 0 ) != recvMsgSize )
       	if( send( clntSock, answer, 256, 0 ) != 256 )
        {
            perror( "send" );
            exit( 1 );
        }

   	    close(clntSock);    /* Fecha o socket desse cliente */
    }

    /* Fecha o socket do servidor, mas nesse exemplo a execucao numa chegara ateh aqui. */
    close(servSock);

}

int main( int argc, char** argv )
{

	if( argc < 5 )    /* Testa se o numero de argumentos esta correto */
    {
        fprintf(stderr, "Usage: %s <name> <broadcast> <udp port> <tcp port>\n", argv[0] );
        exit(1);
    }

	pthread_t heartbeat, update, client;
	struct sharedData shared;
	//debug
	struct neighbor n;
	
	//todo add my ip at list
	//question.. pode add o ip aos parametros


	shared.mutex = PTHREAD_MUTEX_INITIALIZER;
	strcpy( shared.name, argv[1] );
	strcpy( shared.broadcast, argv[2] );
	shared.tcpPort = atoi( argv[3] );
	shared.tcpPort = atoi( argv[4] );

	strcpy( n.ip, "192.168.100.xxx" );
	strcpy( n.name, "localhost" );
	shared.neigh.push_back( n );

	char* servIP;						/* IP address of server */
	unsigned short udpPort;				/* Echo server port */
	char* echoString;

	servIP 		 = argv[2];				// Broadcast ip, hard-coded value
	udpPort = atoi( argv[3] );		 	// Server ports, hard-coded value

	echoString = (char*)malloc( 6 );	/* Third arg: string to echo */
	strcpy( echoString, "START" );

	sendUdpCmd( servIP, udpPort, echoString );

	//if( pthread_create( &heartbeat, NULL, update_function, &shared ) )
    //{
    //	cout << "Error to creating thread heartbeat!" << endl;
    //    return -1;
    //}
    //if( pthread_create( &update, NULL, update_function, &shared ) )
    //{
    //	cout << "Error to creating thread update!" << endl;
    //    return -1;
    //}
	if( pthread_create( &client, NULL, recvClientCommands, &shared ) )
    {
    	cout << "Error to creating thread client!" << endl;
        return -1;
    }

    if( pthread_join( client, NULL ) )
    {
        fprintf( stderr, "Error joining thread\n" );
        return -1;
    }

	return 0;
}
