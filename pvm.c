#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

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

// Driver code
int main(int argc, char *argv[])
{
    FILE* ptr;
    char ch;

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
        unsigned long pid = strtoul(argv[2], NULL, 0);

        FILE *fp;
        char word[40];
        char buf[50];
        snprintf(buf, 50, "/proc/%lu/maps", pid);
        fp = fopen(buf, "r");
        if(fp == NULL){
            printf("Error opening the file!\n");
            return 1;
        } else{
            char hexDigits[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
            int sumOfVirtualinKB = 0;
            int exclusive_page_count = 0;
            int all_pages_count = 0;

            char filename[60];
            snprintf(filename, 60, "/proc/%lu/pagemap", pid);
            int fd = open(filename, O_RDONLY);
            if (fd == -1) {
                perror("Error opening file");
                return 1;
            }

            int fx = open("/proc/kpagecount", O_RDONLY);
            if (fx == -1) {
                perror("Error opening file");
                return 1;
            }

            while(!feof(fp)){
                fscanf(fp, "%s%*[^\n]", word);

                char *token = strtok(word, "-");
                char start[20];
                char end[20];
                int choice = 0;
                while(token != NULL){
                    if(choice == 0){
                        strcpy(start, token);
                        choice = 1;
                    }
                    else if(choice == 1){
                        strcpy(end, token);
                        choice = 0;
                    }
                    token = strtok(NULL, " ");
                }

                if(strlen(start) > 2 && strlen(end) > 2 && strlen(start) != 16 && strlen(end) != 16){
                    unsigned long decimalStart = 0;
                    unsigned long decimalEnd = 0;    

                    int i, j, power = 0, digit;
                    for(i = strlen(start) - 1; i >= 0; i--){
                        for(j = 0; j < 16; j++){
                            if(start[i] == hexDigits[j]){
                                decimalStart += j * pow(16, power);
                            }
                        }
                        power++;
                    }

                    int x, y, power2 = 0, digit2;
                    for(x = strlen(end) - 1; x >= 0; x--){
                        for(y = 0; y < 16; y++){
                            if(end[x] == hexDigits[y]){
                                decimalEnd += y * pow(16, power2);
                            }
                        }
                        power2++;
                    }

                    sumOfVirtualinKB = sumOfVirtualinKB + (decimalEnd - decimalStart)/1024;

                    int numOfPagesSpanend = (decimalEnd - decimalStart)/4096;
                    for(int k = 0; k < numOfPagesSpanend; k++){
                        uint64_t pagemapRaw = 0;
                        uint64_t startDecimal = decimalStart;
                        uint64_t vpn = vmem_getVPN(startDecimal);
                        pagemapRaw = getPagemapRaw(fd, startDecimal + (k * 4096));
                        printf("pagemapRaw: %lu\n", pagemapRaw);
            
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

                        off_t temp2 = lseek(fx, (long)decimalPFN * 8, SEEK_SET);
                        
                        unsigned long page_count;
                        ssize_t bytesRead2 = read(fx, &page_count, sizeof(page_count));

                        if(page_count == 1){
                            exclusive_page_count++;
                            all_pages_count++;
                        }
                        else if(page_count > 1){
                            all_pages_count++;
                        }
                        else{
                            continue;
                        }
                    }
                }

                strcpy(start, "");
                strcpy(end, "");
                strcpy(word, "");
            }

            close(fd);
            close(fx);

            int exclusive_result = exclusive_page_count * 4;
            int all_pages_result = all_pages_count * 4;

            printf("Virtual memory usage/size is: %d KB \n", sumOfVirtualinKB);
            printf("Physical memory usage/size for EXCLUSIVELY MAPPED PAGES is: %d KB \n", exclusive_result);
            printf("Physical memory usage/size for ALL PROCESSPAGES is: %d KB \n", all_pages_result);
        }

        fclose(fp);    
    }
    else if (strcmp(argv[1], "-mapva") == 0){
        pid_t pid = strtoul(argv[2], NULL, 0);
        uint64_t virtual_address = strtoul(argv[3], NULL, 0);

        char pagemap_path[256];
        snprintf(pagemap_path, sizeof(pagemap_path), "/proc/%d/pagemap", pid);

        int pagemap_fd = open(pagemap_path, O_RDONLY);
        if (pagemap_fd == -1) {
            perror("Failed to open pagemap file");
            exit(EXIT_FAILURE);
        }

        uint64_t pagemapRaw = 0;
        uint64_t vpn = vmem_getVPN(virtual_address);
        pagemapRaw = getPagemapRaw(pagemap_fd, virtual_address);
        if(pagemapRaw == 0){
            printf("Page is not found, perhaps doesn't exist!..");
            close(pagemap_fd);
            return 1;
        }
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

        int binaryVA[64], o;
        uint64_t virtualAddrCopy = virtual_address;
        for(o = 0; virtualAddrCopy > 0; o++){
            binaryVA[o] = virtualAddrCopy%2;
            virtualAddrCopy = virtualAddrCopy / 2;
        }

        int vaOffsetBinary[12];
        for(int k = 0; k < 12; k++){
            vaOffsetBinary[k] = binaryVA[k];
        }

        uint64_t decimalVAOffset = binaryToDecimal(vaOffsetBinary, 12);
        uint64_t physical_address_decimal = decimalPFN + decimalVAOffset;

        printf("Physical Address: %#016lx\n", physical_address_decimal);

        close(pagemap_fd);
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

        uint64_t pagemapRaw = 0;
        uint64_t vpn = vmem_getVPN(virtual_address);
        pagemapRaw = getPagemapRaw(pagemap_fd, virtual_address);
        if(pagemapRaw == 0){
            printf("Page is not found, perhaps doesn't exist!..");
            close(pagemap_fd);
            return 1;
        }
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

    if(strcmp(argv[1], "-maprange") == 0){
        long va1 = (long)strtol(argv[3], NULL, 16);
        long va2 = (long)strtol(argv[4], NULL, 16);
        long tempVa = va1;
        long offset;
        int i = 0;
        long frameNum = 0;
        long pageNum = 0;
        long fd;
        unsigned char buffer[64];
        bool unused = true;
        
        char la[20];
        char ua[20];
        bool first = true;
        bool read1 = true;

        long laH;
        long uaH;

        char *dir = (char*) malloc(50*sizeof(char));
        char *dir2 = (char*) malloc(50*sizeof(char));

        strcat(dir, "/proc/");
        strcat(dir, argv[2]);
        strcat(dir2, dir);
        strcat(dir2, "/pagemap");
        strcat(dir, "/maps");

        fd = open(dir2, O_RDONLY);

        while(tempVa < va2){
            offset = tempVa % 0x1000;
            lseek(fd, offset, SEEK_SET);

            frameNum = 0;
            unused = true;

            read(fd, buffer, 64);
            for(int j = 0; j<64; j++)
                frameNum += (pow(2, (63 - j))) * buffer[j];

            pageNum = tempVa / 0x1000;

            ptr = fopen(dir, "r");
            
            if (NULL == ptr) {
                printf("file can't be opened \n");
            }
        
            do {
                ch = fgetc(ptr);

                if(read1){
                    if(ch == '-'){
                        first = false;
                        ch = fgetc(ptr);
                        i = 0;
                    }

                    if(first){
                        la[i] = ch;
                    }
                    else{
                        ua[i] = ch;
                    }
                    i++;
                }
                if(ch == ' ' && read1){
                    laH = (long)strtol(la, NULL, 16);
                    uaH = (long)strtol(ua, NULL, 16);

                    //printf("@%lx - %lx \n", laH, uaH);
                    //printf("-%lx - %lx \n", va1, va2);
                    //printf("%d - %d\n", (laH > va1), (uaH < va2));

                    if(laH <= tempVa && uaH > tempVa){
                        unused = false;
                    }
                    
                    read1 = false;
                    first = true;
                }
                else if (ch == '\n'){
                    strncpy(la, "", 20);
                    strncpy(ua, "", 20);
                    i = 0;
                    read1 = true;
                }
            } while (ch != EOF);

            if(unused)
                printf("PN: %ld, FN: unused\n", pageNum);
            else if( frameNum == 0)
                printf("PN: %ld, FN: not-in-memory\n", pageNum);
            else
                printf("PN: %ld, FN: %ld\n", pageNum, frameNum);

            tempVa += 0x1000;
        }

        close(fd);

        free(dir);
        free(dir2);
    }
    else if(strcmp(argv[1], "-mapall") == 0){
        long tempVa;
        long offset;
        int i = 0;
        long frameNum = 0;
        long pageNum = 0;
        long fd;
        unsigned char buffer[64];
        
        char la[20];
        char ua[20];
        bool first = true;
        bool read1 = true;

        long laH;
        long uaH;

        char *dir = (char*) malloc(50*sizeof(char));
        char *dir2 = (char*) malloc(50*sizeof(char));

        strcat(dir, "/proc/");
        strcat(dir, argv[2]);
        strcat(dir2, dir);
        strcat(dir2, "/pagemap");
        strcat(dir, "/maps");

        fd = open(dir2, O_RDONLY);

        ptr = fopen(dir, "r");
                    
        if (NULL == ptr) {
            printf("file can't be opened \n");
        }

        do {
            ch = fgetc(ptr);

            if(read1){
                if(ch == '-'){
                    first = false;
                    ch = fgetc(ptr);
                    i = 0;
                }

                if(first){
                    la[i] = ch;
                }
                else{
                    ua[i] = ch;
                }
                i++;
            }
            if(ch == ' ' && read1){
                laH = (long)strtol(la, NULL, 16);
                uaH = (long)strtol(ua, NULL, 16);

                tempVa = laH;

                while(tempVa < uaH){
                    offset = tempVa % 0x1000;
                    lseek(fd, offset, SEEK_SET);

                    frameNum = 0;

                    read(fd, buffer, 64);
                    for(int j = 0; j<64; j++)
                        frameNum += (pow(2, (63 - j))) * buffer[j];

                    pageNum = tempVa / 0x1000;
                
                    if( frameNum == 0)
                        printf("PN: %ld, FN: not-in-memory\n", pageNum);
                    else
                        printf("PN: %ld, FN: %ld\n", pageNum, frameNum);

                    tempVa += 0x1000;
                }
                
                read1 = false;
                first = true;
            }
            else if (ch == '\n'){
                strncpy(la, "", 20);
                strncpy(ua, "", 20);
                i = 0;
                read1 = true;
            }
        } while (ch != EOF);

        close(fd);

        free(dir);
        free(dir2);
    }
    else if(strcmp(argv[1], "-mapallin") == 0){
        long tempVa;
        long offset;
        int i = 0;
        long frameNum = 0;
        long pageNum = 0;
        long fd;
        unsigned char buffer[64];

        char la[20];
        char ua[20];
        bool first = true;
        bool read1 = true;

        long laH;
        long uaH;

        char *dir = (char*) malloc(50*sizeof(char));
        char *dir2 = (char*) malloc(50*sizeof(char));

        strcat(dir, "/proc/");
        strcat(dir, argv[2]);
        strcat(dir2, dir);
        strcat(dir2, "/pagemap");
        strcat(dir, "/maps");

        fd = open(dir2, O_RDONLY);

        ptr = fopen(dir, "r");
                    
        if (NULL == ptr) {
            printf("file can't be opened \n");
        }

        do {
            ch = fgetc(ptr);

            if(read1){
                if(ch == '-'){
                    first = false;
                    ch = fgetc(ptr);
                    i = 0;
                }

                if(first){
                    la[i] = ch;
                }
                else{
                    ua[i] = ch;
                }
                i++;
            }
            if(ch == ' ' && read1){
                laH = (long)strtol(la, NULL, 16);
                uaH = (long)strtol(ua, NULL, 16);

                tempVa = laH;

                while(tempVa < uaH){
                    offset = tempVa % 0x1000;
                    lseek(fd, offset, SEEK_SET);

                    frameNum = 0;

                    read(fd, buffer, 64);
                    for(int j = 0; j<64; j++)
                        frameNum += (pow(2, (63 - j))) * buffer[j];

                    pageNum = tempVa / 0x1000;
                
                    if( frameNum != 0)
                        printf("PN: %ld, FN: %ld\n", pageNum, frameNum);

                    tempVa += 0x1000;
                }
                
                read1 = false;
                first = true;
            }
            else if (ch == '\n'){
                strncpy(la, "", 20);
                strncpy(ua, "", 20);
                i = 0;
                read1 = true;
            }
        } while (ch != EOF);

        close(fd);

        free(dir);
        free(dir2);
    }
    else if(strcmp(argv[1], "-alltablesize") == 0){
        int i = 0;
        unsigned char buffer[64];
        long sum = 0;
        
        char la[20];
        char ua[20];
        bool first = true;
        bool read1 = true;

        long laH;
        long uaH;

        char *dir = (char*) malloc(50*sizeof(char));
        char *dir2 = (char*) malloc(50*sizeof(char));

        strcat(dir, "/proc/");
        strcat(dir, argv[2]);
        strcat(dir, "/maps");

        ptr = fopen(dir, "r");
                    
        if (NULL == ptr) {
            printf("file can't be opened \n");
        }

        do {
            ch = fgetc(ptr);

            if(read1){
                if(ch == '-'){
                    first = false;
                    ch = fgetc(ptr);
                    i = 0;
                }

                if(first){
                    la[i] = ch;
                }
                else{
                    ua[i] = ch;
                }
                i++;
            }
            if(ch == ' ' && read1){
                laH = (long)strtol(la, NULL, 16);
                uaH = (long)strtol(ua, NULL, 16);

                sum += ((uaH - laH) / pow(2, 21) ) + 1;
                
                read1 = false;
                first = true;
            }
            else if (ch == '\n'){
                strncpy(la, "", 20);
                strncpy(ua, "", 20);
                i = 0;
                read1 = true;
            }
        } while (ch != EOF);

        sum += (sum / pow(2, 9) + 1) + (sum / pow(2, 18) + 1) + (sum / pow(2, 27) + 1) + (sum / pow(2, 36) + 1);

        printf("%ld kb\n", (sum * 4));

        free(dir);
    }
    
    fclose(ptr);

    return 0;
}