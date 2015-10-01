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
#include <signal.h>

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

struct con_str {
    char data[255];
    struct con_str *next;
}con_str;

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
void HTML_index(int, struct dir_data*);
struct con_str* HTML_print_table(FILE*, struct con_str*, struct dir_data*);
void HTML_file();
struct dir_data find_list(char*, struct dir_data*);
void find_dir(int, char*, struct dir_data);
char *replaceAll(char *s, const char *olds, const char *news);
int isFileOrDir(char* s);


int main(int argc, char *argv[])
{
    int opt;
    int opt_ok;
    int port_num;
    //Ignore BROKEN PIPE
    signal(SIGPIPE, SIG_IGN);
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
    //Setting templates directory open
    dir_return->next = (struct dir_data*)malloc(sizeof(dir_data));
    dir_return = dir_return->next;
    strncpy (dir_return->sname, "statics", strlen("statics"));
    strncpy (dir_return->lname, "./templates/statics", strlen("./templates/statics"));
    dir_return->num = 1;
    dir_return_origin->num += 1;

    //File Close
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
        HTML_index(sock_clientfd, dir_in_data);
        return;
    }
    //Check other page
    block = strtok(data, "/");
    if (block == NULL){
        return;
    }
    printf("block: %s\n", block);
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
        printf("%s\n", sname);
        printf("%s\n", lname);
        strncpy(dir_list[n].sname, sname, strlen(sname));
        strncpy(dir_list[n].lname, lname, strlen(lname));
    }

    for ( n=0; n<dir_num; n++) {     
        printf("============\n");
        printf("%s\n", block);
        printf("%s\n", dir_list[n].sname);
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
    struct dirent *dir_entry;
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
    
    closedir(dp);
    //Check if file or dir
    if (isFileOrDir(full_dir) == 0){
    //if dir
        printf("DIR!!\n");
        HTML_dir(sock_clientfd, full_dir);
    } else {
    //if file
        printf("DIR: %s\n", full_dir);
        HTML_file(sock_clientfd, full_dir);
    }
}


void send_header(int sock_clientfd, int is_html)
{
    char buffer[255];

    sprintf(buffer, "HTTP/1.0 200 OK\r\n");
    if (send(sock_clientfd, buffer, strlen(buffer), 0) == -1) return;
    sprintf(buffer, "Host: mango.com\r\n");
    if (send(sock_clientfd, buffer, strlen(buffer), 0) == -1) return;
    if (is_html == 1) {
        sprintf(buffer, "Content-Type: text/html;charset=UTF-8\r\n");
    } else {
        sprintf(buffer, "Content-Type: Application/octet-stream;charset=UTF-8\r\n");
    }
    if (send(sock_clientfd, buffer, strlen(buffer), 0) == -1) return;
    sprintf(buffer, "\r\n");
    if (send(sock_clientfd, buffer, strlen(buffer), 0) == -1) return;
}

int error_404(int sock_client)
{
    char buffer[255];

    sprintf(buffer, "HTTP/1.0 200 NOT FOUND\r\n");
    if (send(sock_client, buffer, strlen(buffer), 0) == -1) return;
    sprintf(buffer, "Host:mango.com\r\n");
    if (send(sock_client, buffer, strlen(buffer), 0) == -1) return;
    sprintf(buffer, "Content-Type:text/html;charset=UTF-8\r\n");
    if (send(sock_client, buffer, strlen(buffer), 0) == -1) return;
    sprintf(buffer, "\r\n");
    if (send(sock_client, buffer, strlen(buffer), 0) == -1) return;
    sprintf(buffer, "\r\n");
    if (send(sock_client, buffer, strlen(buffer), 0) == -1) return;
    sprintf(buffer, "<html>404</html>\r\n");
    if (send(sock_client, buffer, strlen(buffer), 0) == -1) return;
    sprintf(buffer, "\r\n");
    if (send(sock_client, buffer, strlen(buffer), 0) == -1) return;
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


void HTML_index(int sock_clientfd, struct dir_data *dir_in_data)
{
    char buffer[255] = {0};
    FILE *fp;
    int SIZE = 255;
    int byte_read;
    int is_html;
    int n;
    struct con_str *chunk;
    struct con_str *chunk_org;

    chunk = (struct con_str*)malloc(sizeof(con_str));
    chunk_org = chunk;

    fp = fopen("templates/index.html", "r");

    //It it is html
    is_html = 1;
    //Basic Socket Header
    send_header(sock_clientfd, is_html);

    printf("INDEX: %s\n", dir_in_data->next->sname);
    //Read file and send
    //while ( (byte_read=read(fp, buffer, sizeof(buffer)))>0 ){
    while ( fgets(buffer, sizeof(buffer), fp) != NULL){
        printf("%s\n",buffer);

        //READ TABLE
        if (strstr(buffer, "{%TABLE_START%}") != NULL) {
            chunk = HTML_print_table(fp, chunk, dir_in_data);
        } else {
            strncpy(chunk->data, buffer, sizeof(buffer));
            //Save in Linked List
            chunk->next = (struct con_str*)malloc(sizeof(con_str));
            chunk = chunk->next;
        }

    }

    //Send to browser with Linked List
    chunk = chunk_org;
    while (chunk->next != NULL){
        printf("send: %s\n",chunk->data);
        if (send(sock_clientfd, chunk->data, sizeof(chunk->data), 0) == -1) break;
        chunk = chunk->next;
    }
    printf("send_END\n");
    close(fp);
}

//Template MADE
struct con_str* HTML_print_table(FILE *fp, struct con_str *chunk, struct dir_data *dir_in_data)
{
    //If File or Dir
    //Get name, Modified
    //If File, Filesize

    char buffer[255], itos[128];
    char *replace;
    int number=1;
    struct con_str *table_chunk;
    struct con_str *table_chunk_org;
    table_chunk = (struct con_str*)malloc(sizeof(con_str));
    table_chunk_org = table_chunk;

    fgets(buffer, sizeof(buffer), fp);
    while (strstr(buffer, "{%TABLE_END%}") == NULL) {
        printf("DEBUG_FGET: %s\n", buffer);
        strncpy(table_chunk->data, buffer, sizeof(buffer));
        table_chunk->next = (struct con_str*)malloc(sizeof(con_str));
        table_chunk = table_chunk->next;
        fgets(buffer, sizeof(buffer), fp);
    }

    while (dir_in_data->next != NULL){
        table_chunk = table_chunk_org;
        strncpy(buffer, table_chunk->data, sizeof(buffer));
        while (table_chunk->next != NULL) {
            if (strstr(buffer, "{{NUMBER}}") != NULL) {
                sprintf(itos, "%d", number);
                replace = replaceAll(buffer, "{{NUMBER}}", itos);
                strncpy(buffer, replace, sizeof(buffer));
                number++;
                printf("DEBUG_NUM_: %s\n", buffer);
                continue;
            }
            if (strstr(buffer, "{{FILENAME}}") != NULL) {
                replace = replaceAll(buffer, "{{FILENAME}}", dir_in_data->next->sname);
                strncpy(buffer, replace, sizeof(buffer));
                printf("DEBUG_FILE_: %s\n", buffer);
                continue;
            }
            if (strstr(buffer, "{{FILESIZE}}") != NULL) {
            }
            if (strstr(buffer, "{{MODIFIED}}") != NULL) {
            }

            printf("%s\n",buffer);
            strncpy(chunk->data ,buffer, sizeof(buffer));
            chunk->next = (struct con_str*)malloc(sizeof(con_str));
            chunk = chunk->next;
            table_chunk = table_chunk->next;
            strncpy(buffer, table_chunk->data, sizeof(buffer));

        }
        dir_in_data = dir_in_data->next;
    }

    free(table_chunk);
    return chunk;

}


void HTML_file(int sock_clientfd, char* full_dir)
{
    char buffer[255] = {0};
    FILE *fp;
    int SIZE = 255;
    int byte_read;
    int is_html;
    int a=0;

    fp = open(full_dir, 0);

    //It is NOT html
    is_html = 0;
    //Basic Socket Header
    send_header(sock_clientfd, is_html);

    //Read file and send
    while ( (byte_read=read(fp, buffer, sizeof(buffer)))>0 ){
        if (a = send(sock_clientfd, buffer, byte_read, 0) < 0) {
            break;
        }
    }
    close(fp);

}



void HTML_dir(int sock_clientfd, char* full_dir)
{
    char buffer[255] = {0};
    FILE *fp;
    DIR *dp;
    int SIZE = 255;
    int byte_read;
    int is_html;
    int n;
    struct con_str *chunk;
    struct con_str *chunk_org;
    struct dir_data *dir_in_data;
    struct dir_data *dir_in_data_org;
    struct dirent *dir_entry;

    chunk = (struct con_str*)malloc(sizeof(con_str));
    chunk_org = chunk;

    //Make dir_in_data
    dir_in_data = (struct dir_data*)malloc(sizeof(dir_data));
    dir_in_data_org = dir_in_data;

    dp = opendir(full_dir);
    while( dir_entry = readdir( dp )) {
        printf( "%s\n", dir_entry->d_name);
        if (dir_entry->d_name[0] == '.' && dir_entry->d_name[1] == '\0'){
            continue;
        }
        dir_in_data->next = (struct dir_data*)malloc(sizeof(dir_data));
        dir_in_data = dir_in_data->next;
        strncpy(dir_in_data->sname, dir_entry->d_name, sizeof(dir_in_data->sname));
        strncpy(dir_in_data->lname ,full_dir, sizeof(full_dir));
        strncat(dir_in_data->lname, dir_entry->d_name, sizeof(dir_entry->d_name));
    }
    closedir(dp);


    fp = fopen("templates/index.html", "r");

    //It it is html
    is_html = 1;
    //Basic Socket Header
    send_header(sock_clientfd, is_html);

    dir_in_data = dir_in_data_org;
    //Read file and send
    //while ( (byte_read=read(fp, buffer, sizeof(buffer)))>0 ){
    while ( fgets(buffer, sizeof(buffer), fp) != NULL){
        printf("%s\n",buffer);

        //READ TABLE
        if (strstr(buffer, "{%TABLE_START%}") != NULL) {
            chunk = HTML_print_table(fp, chunk, dir_in_data);
        } else {
            strncpy(chunk->data, buffer, sizeof(buffer));
            //Save in Linked List
            chunk->next = (struct con_str*)malloc(sizeof(con_str));
            chunk = chunk->next;
        }

    }

    //Send to browser with Linked List
    chunk = chunk_org;
    while (chunk->next != NULL){
        if (send(sock_clientfd, chunk->data, sizeof(chunk->data), 0) == -1) break;
        chunk = chunk->next;
    }
    close(fp);
    free(dir_in_data);
    free(chunk);
}


char *replaceAll(char *s, const char *olds, const char *news) {
  char *result, *sr;
  size_t i, count = 0;
  size_t oldlen = strlen(olds); if (oldlen < 1) return s;
  size_t newlen = strlen(news);


  if (newlen != oldlen) {
    for (i = 0; s[i] != '\0';) {
      if (memcmp(&s[i], olds, oldlen) == 0) count++, i += oldlen;
      else i++;
    }
  } else i = strlen(s);


  result = (char *) malloc(i + 1 + count * (newlen - oldlen));
  if (result == NULL) return NULL;


  sr = result;
  while (*s) {
    if (memcmp(s, olds, oldlen) == 0) {
      memcpy(sr, news, newlen);
      sr += newlen;
      s  += oldlen;
    } else *sr++ = *s++;
  }
  *sr = '\0';

  return result;
}

int isFileOrDir(char* s)
{
    DIR* dir_info;
    int result;

    printf("CHECK=====\n");
    printf("%s\n", s);
    dir_info = opendir(s);
    if (dir_info != NULL) {
        result = 0; //Directory
        closedir(dir_info);
    } else {
        result = -1; //File or Doesn't exist
    }

    return result;
}
