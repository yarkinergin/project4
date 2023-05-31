#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

uint64_t vmem_pagemap(int pagemap_fd, uint64_t vpn){
    uint64_t retval;
    lseek(pagemap_fd, (long)vpn * 8, SEEK_SET);
    read(pagemap_fd, &retval, sizeof(uint64_t));

    return retval;
}

uint64_t vmem_getVPN(uint64_t addr){
    uint64_t vpn = addr >> 12;
    return vpn;
}

uint64_t getPagemapRaw(int pagemap_fd, uint64_t addr){
    uint64_t pagemapraw = vmem_pagemap(pagemap_fd, addr >> 12);
}

uint64_t binaryToDecimal(int* binary, int length){
    uint64_t result = 0;
    for(int i = 0; i < length; i++){
        result = result + (binary[i] * pow(2,i));
    }
    return result;
}


int main(int argc, char *argv[]){
    //TODO: Hexadecimal-to-decimal converter func lazım. Hex girilirse ilk ceviricez...
    if (strcmp(argv[1], "-frameinfo") == 0){
        unsigned long frame_number = strtoul(argv[2], NULL, 0);
        unsigned long offset = frame_number * 8;
        
        int fd = open("/proc/kpagecount", O_RDONLY);
        if (fd == -1) {
            perror("Error opening file");
            return 1;
        }

        off_t temp = lseek(fd, offset, SEEK_SET);
        if (temp == -1) {
            perror("Error seeking file");
            close(fd);
            return 1;
        }
        
        unsigned long count;
        ssize_t bytesRead = read(fd, &count, sizeof(count));
        if (bytesRead == -1) {
            perror("Error reading file");
            close(fd);
            return 1;
        }

        printf("Number of times page in frame number %lu is mapped: %lu\n", frame_number, count);
        close(fd);

        int fd2 = open("/proc/kpageflags", O_RDONLY);
        if (fd2 == -1) {
            perror("Error opening file");
            return 1;
        }

        off_t temp2 = lseek(fd2, offset, SEEK_SET);
        if (temp2 == -1) {
            perror("Error seeking file");
            close(fd2);
            return 1;
        }

        unsigned long flags;
        ssize_t bytesRead2 = read(fd2, &flags, sizeof(flags));
        if (bytesRead2 == -1) {
            perror("Error reading file");
            close(fd2);
            return 1;
        }

        int a[64], i;
        int n = flags;
        //system("cls");
        for(i = 0; n > 0; i++){
            a[i] = n%2;
            n = n/2;
        }
        
        printf("Flags for the frame number %lu are:\n", frame_number);
        for(int x = 0; x < i; x++){
            if(a[x] == 1){
                printf("%d\n", x);
            }
        }

        close(fd2);
    }
    else if (strcmp(argv[1], "-memused") == 0){
        
    }
    else if (strcmp(argv[1], "-mapva") == 0){

    }
    else if (strcmp(argv[1], "-pte") == 0){
        pid_t pid = strtoul(argv[2], NULL, 0);
        uint64_t virtual_address = strtoul(argv[3], NULL, 0);

        char pagemap_path[256];
        snprintf(pagemap_path, sizeof(pagemap_path), "/proc/%d/pagemap", pid);

        int pagemap_fd = open(pagemap_path, O_RDONLY);
        if (pagemap_fd == -1) {
            perror("Failed to open pagemap file");
            exit(EXIT_FAILURE);
        }

        uint64_t vpn = vmem_getVPN(virtual_address);
        uint64_t pagemapRaw = getPagemapRaw(pagemap_fd, virtual_address);
        uint64_t pagemapRawCopy = pagemapRaw;

        int pagemapBinary[64], i;

        for(i = 0; pagemapRawCopy > 0; i++){
            pagemapBinary[i] = pagemapRawCopy%2;
            pagemapRawCopy = pagemapRawCopy / 2;
        }

        int binaryPFN[55];
        for(int k = 0; k < 55; k++){
            binaryPFN[k] = pagemapBinary[k]; 
        }

        uint64_t decimalPFN = binaryToDecimal(binaryPFN, 55);

        printf("Virtual address: %#016lx\n", virtual_address);
        printf("Physical Frame Number: %#010lx\n", decimalPFN);
        printf("Page Present: %d (", pagemapBinary[63]);
        if(pagemapBinary[63] == 1){
            printf("YES)\n");
        } 
        else if(pagemapBinary[63] == 0){
            printf("NO)\n");
        }
        uint64_t swapOffset = decimalPFN >> 5;
        printf("Swap Offset: %#01lx\n", swapOffset);

        close(pagemap_fd);
    }
    else if (strcmp(argv[1], "-maprange") == 0){
        printf("6\n");
    }
    else if (strcmp(argv[1], "-mapall") == 0){
        printf("7\n");
    }
    else if (strcmp(argv[1], "-mapallin") == 0){
        printf("8\n");
    }
    else if (strcmp(argv[1], "-alltablesize") == 0){
        printf("9\n");
    }

    return 0;
}