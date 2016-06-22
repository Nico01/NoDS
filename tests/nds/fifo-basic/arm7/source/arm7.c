#include <nds.h>

#define IPCFIFOCNT *(unsigned short*)0x4000184
#define IPCFIFOSEND *(unsigned long*)0x4000188

#define FIFO_ENABLE (1 << 15)
#define FIFO_SEND_FULL 2

int main() {
    int i = 0;

    irqInit();
    irqEnable( IRQ_VBLANK);

    // Enable FIFOs
    IPCFIFOCNT = FIFO_ENABLE;

    // Keep the ARM7 mostly idle
    while (1) {
        // Send i over FIFO
        if (!(IPCFIFOCNT & FIFO_SEND_FULL)) {
            IPCFIFOSEND = i++;
        }
        swiWaitForVBlank();
    }

    return 0;
}
