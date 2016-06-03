#ifdef GDB_SUPPORT

#include <string.h>
#include "arm_gdb.h"

arm_gdb* arm_gdb_make(arm_cpu* arm, int port)
{
    arm_gdb* gdb = malloc(sizeof(arm_gdb));
    int client_length = sizeof(struct sockaddr_in);
    // Create the underlying socket
    gdb->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (gdb->socket < 0) {
        LOG(LOG_ERROR, "Cannot create socket.");
    }

    // Setup server address
    memset((char*)&gdb->server, 0, sizeof(struct sockaddr_in));
    gdb->server.sin_family = AF_INET;
    gdb->server.sin_addr.s_addr = INADDR_ANY;
    gdb->server.sin_port = htons(port);

    // Bind server address to socket
    if (bind(gdb->socket, (struct sockaddr*)&gdb->server,
             sizeof(struct sockaddr_in)) < 0) {
        LOG(LOG_ERROR, "Cannot bind on port %d", port);
    }

    LOG(LOG_INFO, "Wait for GDB client on 127.0.0.1:%d", port);

    // Start to listen and accept client
    listen(gdb->socket, 1);
    gdb->socket2 = accept(gdb->socket, (struct sockaddr*)&gdb->client,
                          &client_length);

    // Assign processor to the gdb stub
    gdb->arm = arm;

    LOG(LOG_INFO, "Connection established.");

    return gdb;
}

#endif
