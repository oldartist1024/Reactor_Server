#include <iostream>
#include <unistd.h>
#include "TcpServer.h"
using namespace std;
int main(int argc, char *argv[])
{
#if 0
    if (argc < 3)
    {
        cout << "./a.out port path" << endl;
        return -1;
    }
    uint16_t port = atoi(argv[1]);
    const char *path = argv[2];
    chdir(path);
#else
    uint16_t port = 10000;
    chdir("/home/xu/test");
#endif
    TcpServer *server = tcpServerInit(port, 4);
    tcpServerRun(server);
    return 0;
}