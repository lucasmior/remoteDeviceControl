#include <iostream>		// std
#include <cstring>		// strcpy
#include <vector>		// vector
#include <sstream>		// ostringstream

#include <stdio.h>		/* for printf() and fprintf() */
#include <sys/socket.h>	/* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>	/* for sockaddr_in and inet_addr() */
#include <stdlib.h>		/* for atoi() and exit() */
#include <string.h>		/* for memset() */
#include <unistd.h>		/* for close() */

#include <cstdlib>		// itoa
#include <stdlib.h>		// malloc	
#include <pthread.h>	// pthread
#include <unistd.h>		// sleep

#include <memory.h>

using namespace std;

#define ECHOMAX 255		/* Longest string to echo */
#define MAXPENDING 5    /* Numero maximo de conexoes que podem ficar esperando serem aceitas */
#define RCVBUFSIZE 32   /* Tamanho do buffer de recebimento */

struct neighbor
{
	char ip[16];
	char name[255];
	time_t	lastAccess;
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
	struct sockaddr_in echoServAddr;	/* Echo server addr */

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

	/* Construct the server addr structure */
	memset( &echoServAddr, 0, sizeof(echoServAddr) );	/* Zero out structure */
	echoServAddr.sin_family = AF_INET;	/* Internet addr family */
	echoServAddr.sin_addr.s_addr = inet_addr(servIP);	/* Server IP addr */
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
		//exit( 1 );
	}
	close(sock);
}

void updateServer( struct neighbor n, struct sharedData* shared )
{
	pthread_mutex_lock( &shared->mutex );
	for( unsigned int i = 0; i < shared->neigh.size( ); i++ )
	{
		if( strcmp( shared->neigh.at(i).ip, n.ip ) == 0 )
		{
			strcpy( shared->neigh.at(i).name, n.name );
			shared->neigh.at(i).lastAccess = n.lastAccess;
			pthread_mutex_unlock( &shared->mutex );
			return;
		}
	}
	shared->neigh.push_back( n );

	pthread_mutex_unlock( &shared->mutex );
	return;
}

void *update_function( void *arg )
{
	struct sharedData *shared = (struct sharedData*)arg;
	
	int sock;							/* Socket */
	struct sockaddr_in echoServAddr;	/* Local addr */
	struct sockaddr_in echoClntAddr;	/* Client addr */
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

	/* Construct local addr structure */
	memset( &echoServAddr, 0, sizeof (echoServAddr) );	/* Zero out structure */
	echoServAddr.sin_family = AF_INET;					/* Internet addr family */
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);	/* Any incoming interface */
	echoServAddr.sin_port = htons(udpPort);		/* Local port */

	/* Bind to the local addr */
	if( bind( sock, (struct sockaddr *) &echoServAddr, sizeof (echoServAddr) ) < 0 )
	{
		perror("bind");
		exit(1);
	}
	/* Set the size of the in-out parameter */
	cliAddrLen = sizeof (echoClntAddr);

	while( 1 )
	{
		//cout << "Waiting for cmd.. " << endl;
		/* Block until receive message from a client */
		if( ( recvMsgSize = recvfrom( sock, echoBuffer, ECHOMAX, 0,
					    			  (struct sockaddr *)&echoClntAddr,
					    			   &cliAddrLen)) < 0)
	    {
			perror("recvfrom");
			exit(1);
		}

		echoBuffer[recvMsgSize] = '\0';

		//printf("Handling client %s\n", inet_ntoa( echoClntAddr.sin_addr ) );
		//printf("Message: %s\n", echoBuffer);

		//update neigh
		
		char *cmd;
		char *param;

		cmd = strtok( echoBuffer, " " );
		param = strtok( NULL, " " );

		struct neighbor n;
		strcpy( n.name, param );
		strcpy( n.ip , inet_ntoa( echoClntAddr.sin_addr ) );
		n.lastAccess = time( 0 );
		//cout << "name is: " << n.name <<   endl
		//	 << "ip is: " << n.ip << endl;
			
		if( strcmp( cmd, "START" ) == 0 )
		{
			pthread_mutex_lock( &shared->mutex );
			//verify if dont has the same server at the list
			shared->neigh.push_back( n );

			//send a heartbeat go back
			char *echoString = (char*)malloc( 10 + strlen(shared->name) );	/* Third arg: string to echo */
			strcpy( echoString, "HEARTBEAT " );
			strcat( echoString, shared->name );
			sendUdpCmd( n.ip, udpPort , echoString );

			pthread_mutex_unlock( &shared->mutex );	
		}
		else if( strcmp( cmd, "HEARTBEAT" ) == 0 )
		{
			updateServer( n, shared );	//change to struct
		}
		else
		{
			//nothing, invalid cmd
			;;	
		}
	}
}

void *heartbeat_function( void *arg )
{
	struct sharedData *shared = (struct sharedData*)arg;

	char *servIP;					/* IP addr of server */
	unsigned short udpPort;			/* Echo server port */

	servIP 	= shared->broadcast; 	// Broadcast ip, hard-coded value
	udpPort = shared->udpPort;	 	// Server ports, hard-coded value

	char *echoString;				/* String to send to echo server */

	pthread_mutex_lock( &shared->mutex );
	echoString = (char*)malloc( 6 + strlen(shared->name) );
	strcpy( echoString, "START " );
	strcat( echoString, shared->name );
	pthread_mutex_unlock( &shared->mutex );

	sendUdpCmd( servIP, udpPort, echoString );

	while( 1 )
	{
		pthread_mutex_lock( &shared->mutex );
		echoString = (char*)malloc( 10 + strlen(shared->name) );
		strcpy( echoString, "HEARTBEAT " );
		strcat( echoString, shared->name );
		pthread_mutex_unlock( &shared->mutex );

		sendUdpCmd( servIP, udpPort, echoString );
		sleep( 10 );
	}
	return 0;
}

void *recvClientCommands( void *arg )
{
	struct sharedData *shared = (struct sharedData*)arg;

	int servSock;                    	/* Descritor de socket para o servidor */
    int clntSock;                    	/* Descritor de socket para o cliente */
    struct sockaddr_in echoServAddr; 	/* Struct para o endereco local */
    struct sockaddr_in echoClntAddr; 	/* Struct para o endereco do cliente */
    unsigned short tcpPort;     		/* Numero da porta do servidor */
    unsigned int clntLen;            	/* Tamanho da struct de endereco do cliente */
	char echoBuffer[RCVBUFSIZE];        /* Buffer para a string de eco */
    int recvMsgSize;                    /* Tamanho da mensagem recebida */
    int shutdown = 1;					/* Shutdown flag */

    char *cmd;
    char *param;
    char answer[256];

    tcpPort = shared->tcpPort;  		/* Primeiro argumento:  numero da porta local */

	if( memory_init( ) < 0 )
	{
		cout << "Erro ao inicializar a memoria." << endl;
		exit( 1 );
	}

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

    while( shutdown ) /* Executa para sempre */
    {
    	memset( answer, 0, 256 );
    	
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
        //printf( "Handling client %s\n", inet_ntoa( echoClntAddr.sin_addr ) );

        /* Essa funcao e' chamada para tratar a conexao com o cliente, a qual e' definida pelo socket clntSock */
    
	    /* Recebe uma mensagem do cliente */
	    if( ( recvMsgSize = recv( clntSock, echoBuffer, RCVBUFSIZE, 0 ) ) < 0 )
	    {
	        perror( "recv" );
	        exit( 1 );
	    }
	    echoBuffer[recvMsgSize] = '\0';
	    
	    /* Get the first work from client */
	    cmd = strtok( echoBuffer, " " );
	    

    	if( strcmp( cmd, "NAME" ) == 0 )
    	{

   	    	strcpy( answer, "200 OK\n" );
    		param = strtok( NULL, " " );
    
    		pthread_mutex_lock( &shared->mutex );
    		if( param != NULL )
    		{
    			strcpy( shared->name, param );
    		}
    
    		strcat( answer, shared->name );
    		pthread_mutex_unlock( &shared->mutex );
       	}

    	else if( strcmp( cmd, "WRITE" ) == 0 || strcmp( cmd, "READ" ) == 0 )
    	{
    		//Parse param..
    		char* type = strtok ( NULL, " " );
    		int addr  = atoi( strtok ( NULL, " " ) );
			char* value;
			unsigned int word = 0;
    		unsigned char byte = 0;
				
			strcpy( answer, "200 OK\n" );
    		
    		if( strcmp( cmd, "WRITE" ) == 0 )
    		{
    			value = strtok ( NULL, " " );
    			if( strcmp( type, "byte" ) == 0 )
    			{
		    		byte = atoi( value );
					if( memory_write_byte( addr, &byte ) < 0 )
					{
						cout << "Erro ao escrever byte na memoria." << endl;
						strcpy( answer, "500 Error\n" );
					}
				}
				else if( strcmp( type, "word" ) == 0 )
				{
					word = atoi( value );
					if( memory_write_word( addr, &word ) < 0 )
					{
						cout << "Erro ao escrever word na memoria." << endl;
						strcpy( answer, "500 Error\n" );
					}
				}
				else
				{
					cout << "Tipo invalido." << endl;
					strcpy( answer, "404 Invalid Address\n" );
				}
    		}
    		else
    		{
    			if( strcmp( type, "byte" ) == 0 )
    			{
					if( memory_read_byte( addr, &byte ) < 0 )
					{
						cout << "Erro ao ler byte da memoria." << endl;
						strcpy( answer, "500 Error\n" );
					}
					value = (char*)malloc( 4 );
					sprintf( value, "%d", byte );		
				} 
				else if( strcmp( type, "word" ) == 0 )
				{
					if( memory_read_word( addr, &word ) < 0 )
					{
						cout << "Erro ao ler word da memoria." << endl;
						strcpy( answer, "500 Error\n" );
					}
					value = (char*)malloc( 4 );
					sprintf( value, "%d", word );	
				}
				else
				{
					cout << "Tipo invalido." << endl;
					strcpy( answer, "404 Invalid Address\n" );
				}
    		}
			strcat( answer, type );
			strcat( answer, " " );
			strcat( answer, value );
		}
    	else if( strcmp( cmd, "SHUTDOWN" ) == 0 )
    	{
	    	strcpy( answer, "200 OK\n" );
			shutdown = 0;	    		
    	}
    	else if( strcmp( cmd, "HOSTS" ) == 0 )
    	{
	    	strcpy( answer, "200 OK\n" );
    		pthread_mutex_lock( &shared->mutex );
    		//cout << "Hosts: " << endl;
    		ostringstream buf;
    		buf << shared->neigh.size( );
    		
    		strcat( answer, buf.str( ).c_str( ) );
	    	strcat( answer, "\n" );
    		
    		for( unsigned int i = 0; i < shared->neigh.size( ); i++ )
    		{
    			//cout << "IP: " << shared->neigh.at(i).ip << " name: " << shared->neigh.at(i).name << endl; 
	    		strcat( answer, shared->neigh.at(i).ip );	//error here .xxx
	    		strcat( answer, " " );
	    		strcat( answer, shared->neigh.at(i).name );
	    		strcat( answer, "\n" );
    		}
    		pthread_mutex_unlock( &shared->mutex );
    	}
	    else
	    {
	    	strcpy( answer, "400 Not Found\n" );
	    	//cout << "Invalid Command" << endl;
	    }

	    /* Envia a string recebida de volta ao cliente e recebe outra ate o final da transmissao */
        /* Envia a mensagem de volta ao cliente */

       	int size = strlen(answer);
       	if( send( clntSock, &size, sizeof(int), 0 ) != sizeof(int) )
        {
            perror( "send" );
            exit( 1 );
        }
       	if( send( clntSock, answer, strlen(answer), 0 ) != (unsigned int)strlen(answer) )
        {
            perror( "send" );
            exit( 1 );
        }

   	    close(clntSock);    /* Fecha o socket desse cliente */
    }

    /* Fecha o socket do servidor, mas nesse exemplo a execucao numa chegara ateh aqui. */
    close(servSock);
	return 0;
}

void *timeout( void *arg )
{
	struct sharedData *shared = (struct sharedData*)arg;

	while( 1 )
	{
		pthread_mutex_lock( &shared->mutex );
		for( unsigned int i = 0; i < shared->neigh.size( ); i++ )
		{
			// timeout control
			time_t now = time(0);
			if( ( now - shared->neigh.at(i).lastAccess ) >= 30 )
			{
				//cout << now << " " << shared->neigh.at(i).lastAccess << endl;
				shared->neigh.erase( shared->neigh.begin( ) + i );	//remove the 'i' element
				//cout << "Removed!" << endl;
			}
		}
		pthread_mutex_unlock( &shared->mutex );
		sleep( 1 );
	}
	return 0;
}

int main( int argc, char** argv )
{
	if( argc < 5 )    /* Testa se o numero de argumentos esta correto */
    {
        fprintf(stderr, "Usage: %s <name> <broadcast> <udp port> <tcp port>\n", argv[0] );
        exit(1);
    }

	pthread_t heartbeat, update, client, tm;
	struct sharedData shared;

	shared.mutex = PTHREAD_MUTEX_INITIALIZER;
	strcpy( shared.name, argv[1] );
	strcpy( shared.broadcast, argv[2] );
	shared.udpPort = atoi( argv[3] );
	shared.tcpPort = atoi( argv[4] );

	if( pthread_create( &heartbeat, NULL, heartbeat_function, &shared ) )
    {
    	cout << "Error to creating thread heartbeat!" << endl;
        return -1;
    }
    if( pthread_create( &update, NULL, update_function, &shared ) )
    {
    	cout << "Error to creating thread update!" << endl;
        return -1;
    }
	if( pthread_create( &client, NULL, recvClientCommands, &shared ) )
    {
    	cout << "Error to creating thread client!" << endl;
        return -1;
    }
    if( pthread_create( &tm, NULL, timeout, &shared ) )
    {
    	cout << "Error to creating thread timeout!" << endl;
        return -1;
    }

    if( pthread_join( client, NULL ) )
    {
        fprintf( stderr, "Error joining thread\n" );
        return -1;
    }
	return 0;
}
