#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // Linux的Socket API
#include <netinet/in.h> // 包含相關的頭文件
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h> 


#define BUF_SIZE 4096
#define MAX_CLIENT 5


pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;




// 定義客戶端結構
struct Client {
    int socket;
    pthread_t thread;
    char username[64] ;
};

//目前客戶與所有客戶結構
struct ThreadArgs {
    struct Client *cur_client;
    struct Client **clients;
};

void parser_json( char* jsonstr, char* message, char* name, char* time ) {


    // char* username, char* message, char* timestamp 

    // "{\n"
    // "    \"message_content\": %s,\n"
    // "    \"sender\": {\n"
    // "        \"username\": %s\n"
    // "    },\n"
    // "    \"timestamp\": %s\n"
    // "}\n"


    char *messageStart = strstr(jsonstr, " \"message_content\": ") + strlen(" \"message_content\": ");
    char *messageEnd = strchr(messageStart, ',');
    // char message[1024];
    memset(message, 0, sizeof(message)) ;
    strncpy(message, messageStart, messageEnd - messageStart);


    char *nameStart = strstr(jsonstr, "\"username\": ") + strlen("\"username\": ");
    char *nameEnd = strchr(nameStart, '\n');
    // char name[64];
    memset(name, 0, sizeof(name)) ;
    strncpy(name, nameStart, nameEnd - nameStart);
    


    char *timeStart = strstr(jsonstr, "\"timestamp\": ") + strlen("\"timestamp\": ");
    char *timeEnd = strchr(timeStart, '\n');
    // char time[64];
    memset(time, 0, sizeof(time)) ;
    strncpy(time, timeStart, timeEnd - timeStart);





}



void bulk_send(struct Client *cur_client ,struct Client **clients , char* buffer) { //群發

    // 轉發訊息給其他客戶端
    int i = 0 ;
    char message [1024] ;
    char username[64];
    char timestamp[8] ;
    char allmessage[1024] ;
    
    // time_t timep ; 
    // struct tm * timeinfo ; 

    // time(&timep) ; //1970到現在過了幾秒
    // timeinfo = localtime(&timep) ; //將秒數轉為 struct tm 結構


    

    parser_json( buffer, message, username, timestamp ) ;


    sprintf( allmessage, "@%s : %s                            --%s\n" ,username, message, timestamp );




    for (i = 0; i < MAX_CLIENT; i++) {


        if (clients[i] != NULL && clients[i]->socket != cur_client->socket) { 

            // printf("%s",buffer) ;
            
            send(clients[i]->socket, allmessage, strlen(allmessage), 0);
        }
    }

   



}


void remove_client(int socket, struct Client **clients) {
    int i;
    for (i = 0; i < MAX_CLIENT; i++) {
        if (clients[i] != NULL && clients[i]->socket == socket) {
            free(clients[i]);
            clients[i] = NULL;
            break;
        }
    }
}

void *handle_client( void *arg) {



    struct ThreadArgs *args = (struct ThreadArgs *)arg;  
    //因為pthread_create只接受void*
    struct Client *cur_client = args->cur_client;
    struct Client **clients = args->clients;

    
    char buffer[BUF_SIZE] ;


    // 接收和轉發訊息：接受從客戶端發送來的訊息，然後將該訊息轉發給其他所有已連接的客戶端。

    while ( 1 ) {

        memset(buffer, 0, sizeof(buffer));

        
        
        ssize_t word_received = recv(cur_client->socket, buffer, sizeof(buffer), 0);

        

       
       

        if (word_received  <= 0 ) {

             
            printf("目前使用者@%s已斷開...\n", cur_client->username) ;
            
            pthread_mutex_lock(&clients_mutex);
            close(cur_client->socket) ;
            remove_client(cur_client->socket, clients) ;
            pthread_mutex_unlock(&clients_mutex);
            pthread_exit(NULL); //用來結束當前的線程

            break ;

            
        }
        else {

            if (strcmp(buffer, "quit\n") == 0 ) {


                printf("目前使用者@@%s已斷開...\n", cur_client->username) ;

                pthread_mutex_lock(&clients_mutex);
                close(cur_client->socket) ;
                remove_client(cur_client->socket, clients) ;
                pthread_mutex_unlock(&clients_mutex);

                pthread_exit(NULL); //用來結束當前的線程
                break; 
                
            }


            // printf("word_received : %zu\n", word_received ) ;


            // 判斷訊息的類型
            if (buffer[word_received-1] == '\n') {
                // Received ageneral message 
                // buffer[word_received-1] = '\0';
                // char *newline = strtok(buffer, "\n");
             
                // printf("@%s : %s\n", cur_client->username,buffer);
                
                printf("Received ageneral message  : %s\n", buffer);

                //將接收到的訊息對所有使用者(除發送者之外)進行群發 

                pthread_mutex_lock(&buffer_mutex);
                bulk_send(cur_client,clients,buffer) ;   
                pthread_mutex_unlock(&buffer_mutex);

               
            } else {
                // Received a username
                printf("Received a username: %s\n", buffer);
                strcpy(cur_client->username, buffer);
            }

        

         
        }



    }

    //釋放資源

    pthread_exit(NULL); //用來結束當前的線程


}





int main () {


    //創建socket並初始化    
    printf("創建socket...\n");
    int server_socket ;
    server_socket = socket(AF_INET, SOCK_STREAM, 0 ) ;
        //AF_INET 表示使用IPv4協議，如果你要使用IPv6，可以使用 AF_INET6。
        //SOCK_STREAM 表示使用TCP協議，如果需要使用UDP，可以使用 SOCK_DGRAM。
        //最後一個參數通常設為0，表示自動選擇默認協議。
     
    if (server_socket == -1 ) {
        perror("socket failed") ;
        exit(EXIT_FAILURE) ;
    }

    // 綁定套接字到IP地址和端口：將伺服器的套接字綁定到一個特定的IP地址和端口上，以便客戶端可以連接。
    printf("綁定socket到IP以及Port...\n");

    struct sockaddr_in addr = {} ;
    addr.sin_family = AF_INET ;
    addr.sin_addr.s_addr = inet_addr("10.100.91.234") ;
        //s_addr : 存儲IPv4地址的32位二進制值
        //inet_addr :　將ip轉為二進位
    addr.sin_port = htons(8080) ;
        // htonos 將Host Byte Order 轉為Network Byte Order

    
    if (bind(server_socket, (struct sockaddr*) &addr, sizeof(addr)) == -1 ) {
        perror("failed bind !\n") ;
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    printf("Socket 已成功綁定IP&Port \n") ;


    // 監聽連接請求：開始監聽來自客戶端的連接請求。
    printf("設置監聽中...\n") ;

    if (listen(server_socket,10) == -1 ) {


        printf("listen failed !\n") ;
        close(server_socket) ;
        exit(EXIT_FAILURE) ;
        
    }
    printf("已成功監聽 !\n") ;



//下客戶的連接將以多線程的方式進行

    // 接受客戶端連接：當有客戶端連接請求時，接受該連接，建立與該客戶端的通信通道。
    struct Client *clients[MAX_CLIENT] ;
    memset(clients, 0, sizeof(clients));

    struct sockaddr_in client_addr = {} ;
    socklen_t cli_len = sizeof(client_addr) ;   //Socket Length Type
    int client_socket ;
    while( 1 ) {

       
        client_socket = accept(server_socket, (struct sockaddr*) &client_addr, &cli_len ) ;
  

        if(client_socket == -1) {

            // printf("client_socket accept failed !\n");
            perror("client_socket accept failed");
            sleep(0.5);
            continue;
            
        }
        printf("成功與使用者連接 ! \n") ;

        //找客戶使用槽放進去
        int free_slot = -1 ; 
        int i = 0 ; 

        for ( i = 0 ; i < MAX_CLIENT ; i++ ) {
            if( clients[i] == NULL ) {
                free_slot = i ; 
                break ;
            }
        }

        if ( free_slot != -1 ) {


            pthread_mutex_lock(&clients_mutex);
            
            clients[free_slot] = malloc( sizeof(struct Client) ) ;
            clients[free_slot]->socket = client_socket ; 

            //建立線程
                // 接收和轉發訊息：接受從客戶端發送來的訊息，然後將該訊息轉發給其他所有已連接的客戶端。

            struct ThreadArgs args;
            args.cur_client = clients[free_slot];
            args.clients = clients;
            pthread_create(&(clients[free_slot]->thread), NULL, handle_client, &args);
            pthread_mutex_unlock(&clients_mutex);
        }

        else {
            printf("已超出最大使用人數 !\n") ;
            close(client_socket) ;

        }
    
    }


    //釋放記憶體 
    int i = 0 ; 
    for (i = 0; i < MAX_CLIENT; i++) {
        if (clients[i] != NULL) {
            // 等待线程退出
            pthread_join(clients[i]->thread, NULL);
            close(clients[i]->socket);
            free(clients[i]);
        }
    }
    
    free(clients) ; //釋放記憶體

    //互斥鎖修毀
    pthread_mutex_destroy(&clients_mutex);
    pthread_mutex_destroy(&buffer_mutex);

    //關閉socket 
    close(server_socket) ;






    return 0 ;








}
