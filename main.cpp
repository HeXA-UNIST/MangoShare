#include <iostream>
using namespace std;

int add_dir();
int del_dir();
int chg_port();
int start_serv();

int main(int argc, char *argv[])
{
    int menu_sel;
    int pid;

    pid=fork();
    switch(pid){
        case -1:
            cout << "Fork Error\n" << endl;
            break;
        case 0:         // child
            break;
        default:        // parent
            return 0;
    }

    chdir("/");
    setsid();

    work();

    return 0;
    cout << "################################" << endl;
    cout << "####     Mango Webshare     ####" << endl;
    cout << "################################" << endl;


    cout << "Reading settings..." <<endl;
    //Read settings.ini and add directory lists, port number

    cout << "What do you want to do?" << endl;
    cout << "1. Add dir\n2. Del dir\n3. Change port\n4. Strat web server" << endl;
    cin >> menu_sel;

    switch (menu_sel) {
        case 1: add_dir();
                break;
        case 2: del_dir();
                break;
        case 3: chg_port();
                break;
        case 4: start_serv();
                break;
    }

    return 0;
}

//Add directory on web server
int add_dir()
{
}

//Delete directory from list on web server
int del_dir()
{
}

//Change web server port
int chg_port()
{
}

//Start web server
int start_serv()
{
}



