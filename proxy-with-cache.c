
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define MAX_BYTES 4096    //max allowed size of request/response
#define MAX_CLIENTS 400     //max number of client requests served at a time
#define MAX_SIZE 200*(1<<20)     //size of the cache
#define MAX_ELEMENT_SIZE 10*(1<<20)     //max size of an element in cache

typedef struct cache_element cache_element;

struct cache_element{
    char* data;         //data stores response
    int len;          //length of data i.e.. sizeof(data)...
    char* url;        //url stores the request
	time_t lru_time_track;    //lru_time_track stores the latest time the element is  accesed
    cache_element* next;    //pointer to next element
};


int port_number = 8080;				// Default Port
int proxy_socketId;					// socket descriptor of proxy server
pthread_t tid[MAX_CLIENTS];         //array to store the thread ids of clients
sem_t seamaphore;	                //if client requests exceeds the max_clients this seamaphore puts the
                                    //waiting threads to sleep and wakes them when traffic on queue decreases
//sem_t cache_lock;			       
pthread_mutex_t lock;               //lock is used for locking the cache


cache_element* head;                //pointer to the cache
int cache_size;             //cache_size denotes the current size of the cache

int main(int argc, char* argv[]){
    int client_socketId, client_len; // client_socketId == to store the client socket id
	struct sockaddr_in server_addr, client_addr; // Address of client and server to be assigned

    sem_init(&seamaphore,0,MAX_CLIENTS); // Initializing seamaphore and lock
    pthread_mutex_init(&lock,NULL); // Initializing lock for cache
    

	if(argc == 2)        //checking whether two arguments are received or not
	{
		port_number = atoi(argv[1]);
	}
	else
	{
		printf("Too few arguments\n");
		exit(1);
	}

	printf("Setting Proxy Server Port : %d\n",port_number);

    //creating the proxy socket
	proxy_socketId = socket(AF_INET, SOCK_STREAM, 0);

	if( proxy_socketId < 0)
	{
		perror("Failed to create socket.\n");
		exit(1);
	}

    //setting the server address

    int reuse =1; //for reusing the port
    if (setsockopt(proxy_socketId, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) { //setting the socket options
        perror("setsockopt(SO_REUSEADDR) failed\n");
    }
    bzero((char*)&server_addr, sizeof(server_addr)); //setting the server address to zero
    server_addr.sin_family = AF_INET; //IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; //INADDR_ANY is used when you don't need to bind a socket to a specific IP.
    server_addr.sin_port = htons(port_number); //htons() converts the port number to network byte order

    //binding the server address to the socket
    if(bind(proxy_socketId, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        perror("Failed to bind the server address to the socket.\n");
        exit(1);
    }
    printf("Binding on port: %d\n",port_number);
    //listening to the client requests
    if(listen(proxy_socketId, MAX_CLIENTS) < 0)
    {
        perror("Failed to listen to the client requests.\n");
        exit(1);
    }
    int i = 0; // Iterator for thread_id (tid) and Accepted Client_Socket for each thread
	int Connected_socketId[MAX_CLIENTS];   // This array stores socket descriptors of connected clients
    while(1)
    {
        bzero((char*)&client_addr, sizeof(client_addr));
        client_len = sizeof(client_addr);
        client_socketId = accept(proxy_socketId, (struct sockaddr *) &client_addr, &client_len); //accepting the client requests
        if(client_socketId < 0)
        {
            perror("Failed to accept the client request.\n");
            exit(1);
        }
        else{
			Connected_socketId[i] = client_socketId; // Storing accepted client into array
		}
        // Getting IP address and port number of client
		struct sockaddr_in* client_pt = (struct sockaddr_in*)&client_addr; //Casts the generic client_addr structure to the specific sockaddr_in type.
		struct in_addr ip_addr = client_pt->sin_addr; 
		char str[INET_ADDRSTRLEN];										// INET_ADDRSTRLEN: Default ip address size:  Converts the IP address from binary to human-readable text (e.g., "192.168.1.1").
		inet_ntop( AF_INET, &ip_addr, str, INET_ADDRSTRLEN );
		printf("Client is connected with port number: %d and ip address: %s \n",ntohs(client_addr.sin_port), str); //Converts the client’s port number from network byte order to host byte order.
		printf("Socket values of index %d in main function is %d\n",i, client_socketId);
		
		i++; 
        
    }
    close(proxy_socketId); //closing the proxy server
    return 0;


}