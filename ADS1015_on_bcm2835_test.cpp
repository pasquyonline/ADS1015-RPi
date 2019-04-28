#include <bcm2835.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <signal.h>

#include "ADS1015.h"

volatile sig_atomic_t done = 0;

void signalterminate(int signum) {
    done = 1;
    printf("ADS1015 test program exit on sigterm %d.\n", signum);
}

void setupSignalAction() {
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = signalterminate;
    sigaction(SIGTERM, &action, NULL);
}

int setup() {
    if (!bcm2835_init()) return 1;
    return 0;
}

int main() {
    if(setup()) {
        printf("Errore inizializzazione bcm2835");
        return 1;
    }

    setupSignalAction();
    ADS1015 ads1015;

    while(done==0) {
        printf(" -> %d\n", ads1015.readADC_SingleEnded(0));
        usleep(500000);
    }

    bcm2835_close();
}
