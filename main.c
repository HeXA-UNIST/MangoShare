#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h>
#include <pthread.h>
#include <ctype.h>
#include <string.h>

int add_dir();
int del_dir();
int chg_port();
int start_serv();
int con_200();
void send_new_thread(int );
int error_404();
int read_socket_line(int sock_clientfd, char *buffer, int size);
void send_normal_page(int sock_clientfd);

struct th_data {
    int sock;
    char ip[16];
    char buf[1024];
};



int main(int argc, char *argv[])
{
    int opt;
    int opt_ok;
    int port_num;
    while((opt = getopt(argc, argv, "adcs")) != -1) 
    {
        switch(opt) 
        { 
            case 'a':
                printf("ADD\n");
                break; 
            case 'd':
                printf("DEL\n");
                break;
            case 'c':
                printf("CHANGE PORT\n");
                //memcpy(port_num, optarg, 16);
                opt_ok = 1;
                break;
            case 's':
                printf("Start Server\n");
                start_serv();
                break;
        }
    }

    return 0;
}


int start_serv()
{
    //Creating Demon...
    /*
    int pid = fork();
    switch (pid)
    {
        case -1:
            fprintf(stderr, "fork filed.\n");
            break;
        case 0:
            fprintf(stderr, "fork succeed.\n");
            break;
        default:
            fprintf(stderr, "child process id : %d\n", pid);
            return 0;
    }
    */

    //Start Web Server in here.
    int sock_serv, sock_clientfd, serv_port = 5000, state, msg_size;
    struct sockaddr_in server_addr;
    char buffer[1024], send_buffer[1000], *send_text;
    char temp[20];
    char th_data[256];
    int len = sizeof(server_addr);
    pthread_t p_thread;

    if ((sock_serv = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error : ");
        exit(0);
    }
    memset(&server_addr, 0x00, sizeof(server_addr));
    server_addr.sin_family = AF_INET;        /* host byte order */
    server_addr.sin_port = htons(serv_port); /* short, network byte order */
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //bzero(&(server_addr.sin_zero), 8);       /* zero the rest of the struct */

    //Binding port
    state = bind(sock_serv , (struct sockaddr *)&server_addr, sizeof(server_addr)); //Socket binding
    if (state == -1) {
        perror("Bind Error - ");
        exit(0);
    }
    state = listen(sock_serv, 5);

    //Receving....
    while (1) {
        sock_clientfd = accept(sock_serv, (struct sockaddr *)&server_addr, &len);
        //inet_ntop(AF_INET, &server_addr.sin_addr.s_addr, temp, sizeof(temp));
        //printf("Server : %s client connected.\n", temp);

        //msg_size = read(sock_clientfd, buffer, 1024);


        if (pthread_create(&p_thread, NULL, send_new_thread, sock_clientfd) == -1)
        {
            perror("thread Create error\n");
            exit(0);
        }

        //printf("Server : %s client closed.\n", temp);
    }
    close(sock_serv);
    return 0;
}


int con_200()
{
}

void send_new_thread(int sock_clientfd)
{
    char buffer[1024];
    char buffer2[1024];
    char method[255];
    int numchars;
    int n=0, i, j;

    //Check GET or POST
    //Read Cookie
    numchars = read_socket_line(sock_clientfd, buffer, sizeof(buffer));
    recv(sock_clientfd, buffer2, 1024, 0); //??? Why I should do it? If I do not that, it doesn't work.

    i=0, j=0;
    while (!isspace(buffer[j]) && (i < sizeof(method) - 1))
    {
        method[i] = buffer[j];
        i++; j++;
    }
    method[i] = '\0';
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        printf("ERROR\n");
        return;
    }

    //IF GET
    //URL Parsing
    

    //IF POST
    //URL Parsing
    //FUNC CHECK


    //Send to browser
    send_normal_page(sock_clientfd);
    close(sock_clientfd);
    return;
}


void send_normal_page(int sock_clientfd)
{
    char buffer[255] = {0};
    FILE *fp;
    char data[255] = {0};
    int SIZE = 255;
    int byte_read;
    fp = open("templates/Oneshot.zip", 0);
    //fp = fopen("templates/AAA", "rb");

    //Basic Socket Header
    sprintf(buffer, "HTTP/1.0 200 OK\r\n");
    send(sock_clientfd, buffer, strlen(buffer), 0);
    sprintf(buffer, "Content-Type: Application/octet-stream;charset=UTF-8\r\n");
    send(sock_clientfd, buffer, strlen(buffer), 0);
    sprintf(buffer, "Host: mango.com\r\n");
    send(sock_clientfd, buffer, strlen(buffer), 0);
    sprintf(buffer, "Connection: close\r\n");
    send(sock_clientfd, buffer, strlen(buffer), 0);
    //sprintf(buffer, "Cotent-Length: 300000\r\n");
    //send(sock_clientfd, buffer, strlen(buffer), 0);
    sprintf(buffer, "\r\n");
    send(sock_clientfd, buffer, strlen(buffer), 0);
    sprintf(buffer, "\r\n");
    send(sock_clientfd, buffer, strlen(buffer), 0);

    //Read file and send
    while ( (byte_read=read(fp, buffer, sizeof(buffer)))>0 ){
        send(sock_clientfd, buffer, byte_read, 0);
    }
    close(fp);
    
}


int error_404()
{
    /*
       sprintf(buffer, "HTTP/1.0 404 PAGE NOT FOUND\r\n");
       send(sock_client, buffer, strlen(buffer), 0);
       sprintf(buffer, "Content-Type:text/html;charset=UTF-8\r\n");
       send(sock_client, buffer, strlen(buffer), 0);
       sprintf(buffer, "Host:mango.com\r\n");
       send(sock_client, buffer, strlen(buffer), 0);
       sprintf(buffer, "Connection:close\r\n");
       send(sock_client, buffer, strlen(buffer), 0);
       sprintf(buffer, "\r\n");
       send(sock_client, buffer, strlen(buffer), 0);
       sprintf(buffer, "<html>404</html>\r\n");
       send(sock_client, buffer, strlen(buffer), 0);
       sprintf(buffer, "\r\n");
       send(sock_client, buffer, strlen(buffer), 0);
     */
}



int read_socket_line(int sock_clientfd, char *buf, int size)
{
    int i=0;
    char c = '\0';
    int n;
    while ((i < size - 1) && (c != '\n')) {
        n = recv(sock_clientfd, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0) {
            if (c == '\r') {
                n = recv(sock_clientfd, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */
                if ((n > 0) && (c == '\n')) {
                    recv(sock_clientfd, &c, 1, 0);
                }
                else {
                    c = '\n';
                }
            }
            buf[i] = c;
            i++;
        }
        else {
            c = '\n';
        }
    }
    buf[i] = '\0';

    return(i);
}







