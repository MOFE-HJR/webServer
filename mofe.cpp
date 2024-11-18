#include "./webserver/webserver.h"

int main(){

    WebServer web(9000,3,60000,false);
    web.start();

    return 0;
}