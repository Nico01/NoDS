#include <nds.h>
#include <stdio.h>

#define IPCFIFOCNT *(unsigned short*)0x4000184
#define IPCFIFORECV *(unsigned long*)0x4100000

#define FIFO_ENABLE (1 << 15)
#define FIFO_RECV_EMPTY (1 << 8)

int main(void) {
    // Enable FIFOs
    IPCFIFOCNT = FIFO_ENABLE;

    consoleDemoInit();
    puts("Simple IPC FIFO demo.");

    while(1) {
        if (!(IPCFIFOCNT & FIFO_RECV_EMPTY)) {
            printf("%d\n", IPCFIFORECV);
        }
        swiWaitForVBlank();
    }

    return 0;
}
