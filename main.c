#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h>

int add_dir();
int del_dir();
int chg_port();
int start_serv();
int con_200();

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

    //Start Web Server in here.
    int sock_serv, sock_client, serv_port = 5000, state, len, msg_size=255;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1024], send_buffer[1000], *send_text;
    char temp[20];

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

    state = bind(sock_serv , (struct sockaddr *)&server_addr, sizeof(server_addr)); //Socket binding
    if (state == -1) {
        perror("Bind Error - ");
        exit(0);
    }
    state = listen(sock_serv, 5);

    while (1) {
        sock_client = accept(sock_serv, (struct sockaddr *)&server_addr, &len);
        inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, temp, sizeof(temp));
        printf("Server : %s client connected.\n", temp);

        msg_size = read(sock_client, buffer, 1024);
        //send_text = "HTTP/1.1 200 OK\r\nServer: MangoServ\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: close\r\n\r\n<html>It Works!</html>";
        //strncpy(send_buffer, send_text, sizeof(send_text));

        sprintf(buffer, "HTTP/1.1 200 OK\r\n");
        send(sock_client, buffer, strlen(buffer), 0);
        sprintf(buffer, "Content-Type:text/html;charset=UTF-8\r\n");
        send(sock_client, buffer, strlen(buffer), 0);
        sprintf(buffer, "Host:mango.com\r\n");
        send(sock_client, buffer, strlen(buffer), 0);
        sprintf(buffer, "Connection:close\r\n");
        send(sock_client, buffer, strlen(buffer), 0);
        sprintf(buffer, "\r\n");
        send(sock_client, buffer, strlen(buffer), 0);
        sprintf(buffer, "<html>It Works!</html>\r\n");
        send(sock_client, buffer, strlen(buffer), 0);
        sprintf(buffer, "\r\n");
        send(sock_client, buffer, strlen(buffer), 0);

        close(sock_client);
        printf("Server : %s client closed.\n", temp);
    }
    close(sock_serv);
    return 0;
}


int con_200()
{
}














