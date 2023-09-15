# 使用C語言建立聊天室 (socket)

# **目標 :**

- 要有名字登入機制
- 聊天視窗需要有傳送者的名字及傳送時間
- 需使用json當作傳送資料的格式, 如果有其他想法請說明
- 撰寫程式流程圖, 並且製作成報告

# 流程圖 :

![socket流程圖.drawio.png](https://github.com/sokai48/c_socket_chatroom/blob/main/socket%E6%B5%81%E7%A8%8B%E5%9C%96drawio.png)

# **撰寫要點 :**

- **linux socket api的使用**
    - socket() : 創建socket並初始化
    - bind() : 綁定socket到IP地址和端口
    - (client)connect() : 客戶端連接到伺服器的 IP 地址和端口
    - (server)listen() : 監聽來自客戶端的連接請求
    - (server)accept() : 接受該連接，建立與該客戶端的通信通道
    - recv() : 接受從客戶端(伺服器)發送來的訊息
    - send() :訊息發送給客戶端(伺服器)
    
- **thread的使用**
    - Server端
        
        > 對應每個客戶端的請求都是以不同的thread處理
        > 
    - Client端
        
        > main thread處理訊息輸出、child thread處理來自server的群發訊息
        > 

- **mutex的使用**
    - Server端
        - clients數組 : 防止同時連接或是同時段開
            
            ```c
            pthread_mutex_lock(&clients_mutex);
            //對clients數組新增新的用戶或是刪除
            //...
            pthread_mutex_ulock(&clients_mutex);
            ```
            
        - send :  防止同時傳送訊息
            
            ```c
            pthread_mutex_lock(&buffer_mutex);
            //將接受的訊息進行群發 
            //bulk_send(cur_client,clients,buffer) ;
            pthread_mutex_ulock(&buffer_mutex);
            ```
            
        - 
    - Client端
        - send/recv : 防止同時傳送又接受訊息
            
            ```c
            pthread_mutex_lock(&mutex);
            //印出群發訊息
            //printf("%s", message ) ;
            pthread_mutex_unlock(&mutex);
            
            pthread_mutex_lock(&mutex);
            //傳送訊息到server
            //send(client_socket, jsonstr, strlen(jsonstr), 0);
            pthread_mutex_unlock(&mutex);
            ```
            
    
- **傳送資料的時間**
    
    ```c
    // 時間
      time_t timep ; 
      struct tm * timeinfo ;
    
    		//輸入訊息之後，紀錄時間
    		time(&timep) ; //1970到現在過了幾秒
    		timeinfo = localtime(&timep) ; //將秒數轉為 struct tm 結構
    ```
    
- **json傳送格式**
    
    ```json
    	   {
               "message_content": "Hello, World!",
                "sender": {
                    "username": "JohnDoe"
                },
                "timestamp": "03:30:25"
            
    }
    ```
    

# 使用方法 **:**

1. 執行 : ./server
2. 執行 : ./client(多人同時)
3. 輸入用戶名稱
4. 開始傳送訊息

# **DEMO展示 :**

1. 開啟SERVER監聽，等待CLIENT連接
    
    ![Untitled](https://github.com/sokai48/c_socket_chatroom/blob/main/Untitled.png)
    
2. 三個CLIENT連接SERVER
    
    ![Untitled](https://github.com/sokai48/c_socket_chatroom/blob/main/Untitled%201.png)
    
3. 各自輸入用戶名稱
    
    ![Untitled](https://github.com/sokai48/c_socket_chatroom/blob/main/Untitled%202.png)
    
4. 各自傳送訊息，並收到其他人的訊息
    
    ![Untitled](https://github.com/sokai48/c_socket_chatroom/blob/main/Untitled%203.png)