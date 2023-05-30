#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

// Driver code
int main(int argc, char *argv[])
{
    FILE* ptr;
    char ch;

    if(strcmp(argv[1], "-maprange") == 0){
        long va1 = (long)strtol(argv[3], NULL, 16);
        long va2 = (long)strtol(argv[4], NULL, 16);
        long tempVa = va1;
        int i = 0;
        long frameNum = 0;
        long pageNum = 0;
        long fd;
        unsigned char buffer[64];
        bool notInMem = true;
        
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
            lseek(fd, tempVa, SEEK_SET);

            frameNum = 0;
            notInMem = true;

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
                        notInMem = false;
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

            if(notInMem)
                printf("PN: %ld, FN: not-in-memory\n", pageNum);
            else if( frameNum == 0)
                printf("PN: %ld, FN: unused\n", pageNum);
            else
                printf("PN: %ld, FN: %ld\n", pageNum, frameNum);

            tempVa += 0x1000;
        }

        close(fd);
        fclose(ptr);

        free(dir);
    }

    /*
    // Opening file in reading mode
    ptr = fopen("/proc/1/pagemap", "r");
 
    if (NULL == ptr) {
        printf("file can't be opened \n");
    }
 
    printf("content of this file are \n");
 
    // Printing what is written in file
    // character by character using loop.
    do {
        ch = fgetc(ptr);


        // Checking if character is not EOF.
        // If it is EOF stop reading.
    } while (ch != EOF);

    //ptr = fopen("/proc/1/pagemap","rb");  // r for read, b for binary


    unsigned char buffer[64];

    long fd, f1;

    fd = open("/proc/1/pagemap", O_RDONLY);

    f1 = lseek(fd, 0x7f4ca0bed000, SEEK_SET);
    printf("--%ld\n", f1);

    read(fd, buffer, 64);
    for(int j = 0; j<64; j++)
        printf("%d ", buffer[j]);


    f1 = lseek(fd, 0, SEEK_CUR);
    printf("--%ld\n", f1);

    close(fd);
    */

    /*
    for(long i = 0; i < 0x562c5b6c5000; i++){
        fread(buffer,sizeof(buffer),1,ptr);

        for(int j = 0; j<64; j++)
            printf("%u ", buffer[j]); // prints a series of bytes

        printf("\n");
    }

    fseek(ptr,0L,70);
    filelen = ftell(ptr);

    printf("\n%ld\n", filelen);
    */
 
    // Closing the file
    return 0;
}