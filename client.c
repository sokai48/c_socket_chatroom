#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // Linux的Socket API
#include <netinet/in.h> // 包含相關的頭文件
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 4096
#define MAX_CLIENT 5


//互斥鎖
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;



void *recv_message ( void * arg ) {

    int client_socket = *((int *)arg);
    // 接收訊息：客戶端需要接收伺服器和其他客戶端發送的訊息。
    char message[1024] ;
    

    while ( 1 ) {
        memset(message, 0, sizeof(message)) ;
        
        ssize_t word_received = recv(client_socket, message, sizeof(message), 0);


    // 顯示訊息：客戶端應該能夠顯示接收到的訊息，包括傳送者的名字和時間戳。



        if ( word_received <= 0 ) {


            printf("你已離開聊天室...\n");
            break;

        }

        else {
            
            pthread_mutex_lock(&mutex);
            printf("%s", message ) ;
            pthread_mutex_unlock(&mutex);
        }


        

    }

    pthread_exit(NULL); //子線程主動退出


}


int main () {


    //創建socket並初始化
    printf("創建socket....\n") ;



    int client_socket = 0 ;
    client_socket = socket(AF_INET, SOCK_STREAM, 0)  ;


    if (client_socket == -1 ) {
        perror("socket failed") ;
        exit(EXIT_FAILURE) ;
    }

    struct sockaddr_in addr = {} ;
    addr.sin_family = AF_INET ;
    addr.sin_addr.s_addr = inet_addr("10.100.91.234") ;
        //s_addr : 存儲IPv4地址的32位二進制值
        //inet_addr :　將ip轉為二進位
    addr.sin_port = htons(8080) ;
        // htonos 將Host Byte Order 轉為Network Byte Order


    // 連接到伺服器：客戶端需要連接到伺服器的 IP 地址和端口。
    if (connect(client_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    printf("已成功連接伺服器 ! \n") ;


//以下會分成兩個線程，分別為(1)主線程 - 發送訊息 (2)子線程 - 接收訊息

    //互斥鎖初始化
    pthread_mutex_init(&mutex, NULL);

    pthread_t recv_thread ; 


    // 接收訊息：客戶端需要接收伺服器和其他客戶端發送的訊息。
    // 顯示訊息：客戶端應該能夠顯示接收到的訊息，包括傳送者的名字和時間戳。
    pthread_create(&recv_thread, NULL, recv_message , &client_socket);





    // 發送訊息：用戶可以輸入訊息並將其發送給伺服器。
    char message[1024] ;
    char jsonstr[1024] ;
    char username[64] ;
    char timestamp[8] ;

    // 時間
    time_t timep ; 
    struct tm * timeinfo ; 
    

    printf("###################################################\n") ;
    printf("############## Please input your Name #############\n") ;
    printf("###################################################\n") ;
    printf("Input : ") ;


    if(fgets(username, sizeof(username), stdin) != NULL) {

        if(strcmp(username,"\n") ==0 ) {

            strcpy(username, "unknown_\n") ;
        }

        char *newline = strtok(username, "\n");
        send(client_socket, username, strlen(username), 0);
        printf("\n") ;
        
        newline = NULL ;

    }




    printf("###################################################\n") ;
    printf("############# Welcome to the Chatroom #############\n") ;
    printf("###################################################\n") ;
    printf("#### Input the message(or key 'quit' to leave ) ###\n") ;
    printf("###################################################\n") ;
    printf("Input : ") ;
    while ( 1 ) {
        memset(message, 0, sizeof(message)) ;
        memset(jsonstr, 0, sizeof(jsonstr)) ;

        
        if(fgets(message, sizeof(message), stdin) == NULL ) { // 從標準輸入獲得訊息
            break ; 
        }



        if ( strcmp(message, "quit\n") == 0 ) {

            //將要斷開的訊息傳給server 
            send(client_socket, "quit\n", strlen("quit\n"), 0);
            close(client_socket);
            pthread_join(recv_thread , NULL);
            break ; 
        }

        if (strcmp(message, "\n") != 0) {

            char *newline = strtok(message, "\n");

            // {
            //     "message_content": "Hello, World!",
            //     "sender": {
            //         "username": "JohnDoe"
            //     },
            //     "timestamp": "03:30:25"
            // }

            time(&timep) ; //1970到現在過了幾秒
            timeinfo = localtime(&timep) ; //將秒數轉為 struct tm 結構

            sprintf(timestamp, "%02d:%02d:%02d",  timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec ) ;

            sprintf(jsonstr,
            "{\n"
            "    \"message_content\": %s,\n"
            "    \"sender\": {\n"
            "        \"username\": %s\n"
            "    },\n"
            "    \"timestamp\": %s\n"
            "}\n",message,username, timestamp) ;

            pthread_mutex_lock(&mutex);
            send(client_socket, jsonstr, strlen(jsonstr), 0);
            pthread_mutex_unlock(&mutex);

            newline = NULL ;

        }




    }


    



    // 主線程等待子線程結束
    pthread_join(recv_thread , NULL);

    //互斥鎖銷毀
    pthread_mutex_destroy(&mutex);

    
    // 斷開連接：用戶應該能夠安全地斷開與伺服器的連接。
    close(client_socket) ;

    return 0 ;

}
