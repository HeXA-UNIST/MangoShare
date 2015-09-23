#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h>
#include <pthread.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>

enum is_get {POST=0, GET=1 };

struct dir_data {
    int num;
    char sname[255];
    char lname[255];
    struct dir_data *next;
}dir_data;

struct th_data {
    int sock;
    struct dir_data *dir_return;
}th_data;

int add_dir();
int del_dir();
int chg_port();
int start_serv();
struct dir_data* read_setting();
int con_200();
void send_new_thread(void* );
int error_404(int );
int read_socket_line(int sock_clientfd, char *buffer, int size);
void send_normal_page(char*, int, enum is_get, struct dir_data*);
void send_header(int, int);
void HTML_index(int );
void HTML_file();
struct dir_data find_list(char*, struct dir_data*);
void find_dir(int, char*, struct dir_data);


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


//This funciton reads settings.ini
//# is comment
//There are file's information and 
struct dir_data *read_setting()
{
    struct dir_data *dir_return;
    struct dir_data *dir_return_origin;
    char buffer[255] = {0};
    char buffer2[255] = {0};
    char *line;
    char *line_sub;
    FILE *fp;
    int SIZE = 255;
    int len;

    dir_return_origin = dir_return;
    dir_return = (struct dir_data*)malloc(sizeof(dir_data));
    dir_return->num = 0;
    dir_return_origin = dir_return;
    printf("%x\n", dir_return);

    //Make Linked List


    //Open file
    fp = fopen("./settings.ini", "r");

    //Read file and send
    while ( getline(&line, &len, fp) != -1) {
        //# is comment
        if (line[0] == '#') {
            continue;
        }
        
        //name: ~~
        //dir: ~~
        if ( strstr(line, "name") == line ) {
            if (getline(&line_sub, &len, fp) == -1) {
                printf("ERROR\n");
                break;
            }
            if ( strstr(line_sub, "dir") == line_sub ) {
                printf("READ!\n");
                strncpy(line, line+5, strlen(line));
                strncpy(line_sub, line_sub+4, strlen(line_sub));
                line[strlen(line)-1] = '\0';
                line_sub[strlen(line_sub)-1] = '\0';
                dir_return->next = (struct dir_data*)malloc(sizeof(dir_data));
                dir_return = dir_return->next;
                strncpy (dir_return->sname, line, strlen(line));
                strncpy (dir_return->lname, line_sub, strlen(line_sub));
                dir_return->num = 1;
                dir_return_origin->num += 1;
                continue;
            } else {
                printf("ERROR\n");
                continue;
            }
        }

        //Else
        printf("ERROR\n");
    }

    fclose(fp);
    return dir_return_origin;
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

    //Read sendind list
    char dir_list[2][255] = {"files", "templates"};

    //Start Web Server in here.
    int sock_serv, sock_clientfd, serv_port = 5000, state, msg_size;
    struct sockaddr_in server_addr;
    struct timeval timeout;
    char buffer[1024], send_buffer[1000], *send_text;
    char temp[20];
    char th_data[256];
    int len = sizeof(server_addr);
    pthread_t p_thread;
    fd_set reads, cpy_reads;
    int fd_max, fd_num;
    
    //Read Settings....
    struct dir_data *dir_return;
    dir_return = read_setting();

    struct th_data th_in_data;

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
    state = bind(sock_serv, (struct sockaddr *)&server_addr, sizeof(server_addr)); //Socket binding
    if (state == -1) {
        perror("Bind Error - ");
        exit(0);
    }
    state = listen(sock_serv, 5);
    FD_ZERO(&reads);
    FD_SET(sock_serv, &reads);

    //Receving....
    while (1) {

        //Select
        cpy_reads = reads;
        timeout.tv_sec = 5;
        timeout.tv_usec = 50000;

        fd_num = select(sock_serv+1, &cpy_reads, 0, 0, &timeout);
        //fd_num=1;
        
        if (fd_num == -1) {
            printf("fd_num : %d\n", fd_num);
            perror("select() error");
            break;
        }
        if (fd_num == 0) {
            continue;
        }

        sock_clientfd = accept(sock_serv, (struct sockaddr *)&server_addr, &len);
        //inet_ntop(AF_INET, &server_addr.sin_addr.s_addr, temp, sizeof(temp));
        //printf("Server : %s client connected.\n", temp);
        
        th_in_data.sock = sock_clientfd;
        th_in_data.dir_return = dir_return;
        if (pthread_create(&p_thread, NULL, send_new_thread, (void*)&th_in_data) == -1)
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

void send_new_thread(void* arg)
{
    struct th_data *th_in_data = (struct th_data*) arg;
    char buffer[1024];
    char buffer2[1024];
    char method[255];
    char url[255];
    int numchars;
    int n=0, i, j;
    int sock_clientfd = th_in_data->sock;
    struct dir_data *dir_in_data = th_in_data->dir_return;
    enum is_get ifget;

    //Check GET or POST
    //Read Cookie
    numchars = read_socket_line(sock_clientfd, buffer, sizeof(buffer));
    recv(sock_clientfd, buffer2, 1024, 0); //??? Why I should do it? If I do not that, it doesn't work.

    i=0, j=0;
    while (!isspace(buffer[j]) && (i < sizeof(method) - 1)) {
        method[i] = buffer[j];
        i++; j++;
    }
    method[i] = '\0';
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST")) {
        printf("ERROR\n");
        return;
    }

    //IF GET
    if (strcasecmp(method, "GET") == 0) {
        printf("GET\n");
        ifget = GET;
    }
    
    //IF POST
    if (strcasecmp(method, "POST") == 0) {
        printf("POST\n");
        ifget = POST;
    }

    //URL Parsing
    i = 0;
    while (isspace(buffer[j]) && (j < sizeof(buffer))) {
            j++;
    }
    while (!isspace(buffer[j]) && (i < sizeof(url) - 1) && (j < sizeof(buffer))) {
        url[i] = buffer[j];
        i++;
        j++;
    }
    url[i] = '\0';
    printf("%s\n", url);
    //FUNC CHECK

    //Send to browser
    send_normal_page(url, sock_clientfd, ifget, dir_in_data);
    close(sock_clientfd);
    return;
}


void send_normal_page(char* url, int sock_clientfd, enum is_get ifget, struct dir_data *dir_in_data)
{
    char buffer[255] = {0};
    FILE *fp;
    char data[255] = {0};
    char *block;
    char *get_char;
    int SIZE = 255;
    int byte_read;
    int is_html;
    struct dir_data dir_find;

    //Check user's session id
    //check_id();
    
    //GET Parameter Check
    if (ifget == GET) {
            strncpy(buffer, url, sizeof(buffer));
            get_char = strchr(buffer, '?');
            if (get_char != NULL) {
                strncpy(data, buffer, get_char-buffer);
                printf("%s\n", data);
            } else {
                strncpy(data, buffer, sizeof(buffer));
            }
    }
    //Check if index page
    if (strcasecmp(data, "/") == 0) {
        HTML_index(sock_clientfd);
        return;
    }
    //Check other page
    block = strtok(data, "/");
    printf("%s\n", block);
    dir_find = find_list(block, dir_in_data);
    if (dir_find.num == 0){
        error_404(sock_clientfd);
    } else{
        find_dir(sock_clientfd, block, dir_find);
    }
    
}

struct dir_data find_list(char *block, struct dir_data *dir_in_data)
{
    //Make array from read data from settings.ini

    struct dir_data *dir_list;
    int n, dir_num = dir_in_data->num;
    struct dir_data dir_return;
    char* sname;
    char* lname;

    dir_list = (struct dir_data*)malloc(sizeof(dir_data)*dir_num);

    for (n=0; n < dir_num; n++){
        dir_in_data = dir_in_data->next;
        sname = dir_in_data->sname;
        lname = dir_in_data->lname;
        printf("%s\n", lname);
        strncpy(dir_list[n].sname, sname, strlen(sname));
        strncpy(dir_list[n].lname, lname, strlen(lname));
    }

    for ( n=0; n<dir_num; n++) {     
        if (strcasecmp(block, dir_list[n].sname) == 0) {
            //Find it!
            printf("Find!\n");
            strncpy(dir_return.sname, dir_list[n].sname, strlen(dir_list[n].sname));
            strncpy(dir_return.lname, dir_list[n].lname, strlen(dir_list[n].lname));
            dir_return.num = 1;
            return dir_return;
        }
    }
    //It isn't..
    printf("404\n");
    dir_return.num = 0;
    return dir_return;
}

void find_dir(int sock_clientfd, char* block, struct dir_data dir_find)
{
    DIR* dp = NULL;
    char* block_final;
    char* full_dir;
    printf("%s\n", dir_find.lname);
    if (dp = opendir(dir_find.lname) == NULL) {
        printf("DIR_OPEN_ERROR\n");
        error_404(sock_clientfd);
        return;
    }

    asprintf(&full_dir,"%s",dir_find.lname);

    while ( (block = strtok(NULL, "/")) != NULL) {
        printf("%s\n", block);
        block_final = block;
        asprintf(&full_dir,"%s/%s",full_dir,block);
        printf("%s\n", full_dir);
        if ((dp = opendir(full_dir)) == NULL) {
            printf("DIR_OPEN_ERROR\n");
            if ( access(full_dir, R_OK) == -1){
                printf("FILE_ERROR\n");
            } else{
                break;
            }
            error_404(sock_clientfd);
            return;
        }
    }
    //Check if file or dir
    
    //if dir
    
    //if file
    printf("%s\n", full_dir);
    HTML_file(sock_clientfd, full_dir);
}


void send_header(int sock_clientfd, int is_html)
{
    char buffer[255];

    sprintf(buffer, "HTTP/1.0 200 OK\r\n");
    send(sock_clientfd, buffer, strlen(buffer), 0);
    sprintf(buffer, "Host: mango.com\r\n");
    send(sock_clientfd, buffer, strlen(buffer), 0);
    if (is_html == 1) {
        sprintf(buffer, "Content-Type: text/html;charset=UTF-8\r\n");
    } else {
        sprintf(buffer, "Content-Type: Application/octet-stream;charset=UTF-8\r\n");
    }
    send(sock_clientfd, buffer, strlen(buffer), 0);
    sprintf(buffer, "\r\n");
    send(sock_clientfd, buffer, strlen(buffer), 0);
}

int error_404(int sock_client)
{
    char buffer[255];

    sprintf(buffer, "HTTP/1.0 200 NOT FOUND\r\n");
    send(sock_client, buffer, strlen(buffer), 0);
    sprintf(buffer, "Host:mango.com\r\n");
    send(sock_client, buffer, strlen(buffer), 0);
    sprintf(buffer, "Content-Type:text/html;charset=UTF-8\r\n");
    send(sock_client, buffer, strlen(buffer), 0);
    sprintf(buffer, "\r\n");
    send(sock_client, buffer, strlen(buffer), 0);
    sprintf(buffer, "\r\n");
    send(sock_client, buffer, strlen(buffer), 0);
    sprintf(buffer, "<html>404</html>\r\n");
    send(sock_client, buffer, strlen(buffer), 0);
    sprintf(buffer, "\r\n");
    send(sock_client, buffer, strlen(buffer), 0);
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


void HTML_index(int sock_clientfd)
{
    char buffer[255] = {0};
    FILE *fp;
    int SIZE = 255;
    int byte_read;
    int is_html;

    fp = open("templates/index.html", 0);

    //It it is html
    is_html = 1;
    //Basic Socket Header
    send_header(sock_clientfd, is_html);

    //Read file and send
    while ( (byte_read=read(fp, buffer, sizeof(buffer)))>0 ){
        send(sock_clientfd, buffer, byte_read, 0);
    }
    close(fp);
}

void HTML_file(int sock_clientfd, char* full_dir)
{
    char buffer[255] = {0};
    FILE *fp;
    int SIZE = 255;
    int byte_read;
    int is_html;

    fp = open(full_dir, 0);

    //It is NOT html
    is_html = 0;
    //Basic Socket Header
    send_header(sock_clientfd, is_html);

    //Read file and send
    while ( (byte_read=read(fp, buffer, sizeof(buffer)))>0 ){
        send(sock_clientfd, buffer, byte_read, 0);
    }
    close(fp);
}




