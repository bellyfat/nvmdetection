/*
 * Created by Taylor on 8/2/2019.
 *
 * Program reads a file's byte information, then uploads this to memory.
 * Ensures file contents will be merged with matching pages in memory if they exist.
 * This program is for testing if the file is uploaded somewhere else.
 * Run advanced_file_upload.c to upload a file only.
 *
 */

//#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>



int main(int argc, const char *argv[]){

    if(argc < 2){
        printf("\nMust input a file name after running program.\n\nExample: [User@somewhere] ./RunProg <filename>\n\n");
        return 1;
    }
    else if (argc > 2){
        printf("\nProgram is designed to only take in one file argument at a time.\n\n");
        return 1;
    }


    FILE *fileptr;
    char *byteArray;
    long filesize;

    fileptr = fopen(argv[1], "rb");       // Opens the file in binary mode
    if (fileptr == NULL){
        printf("\nCould not open file %s!\n\n", argv[1]);
        return 1;
    }
    fseek(fileptr, 0, SEEK_END);          // Jumps to the end of the file
    filesize = ftell(fileptr);            // Gets the current byte offset in the file
    rewind(fileptr);                      // Jumps back to the beginning of the file

    byteArray = (char *)malloc((filesize)*sizeof(char));  // Allocates enough memory for file
    fread(byteArray, filesize, 1, fileptr);               // Reads in the entire file

    //!!!It is crucial to call rewind() after every fread() and fwrite()
    //!!!or you'll get a memory leak.
    rewind(fileptr);
    printf("\nFirst 10 bytes of file: ");
    for (int z = 0; z < 10; z++){
        printf(" %d ", (unsigned) byteArray[z]);
    }
    printf("\n");
    fclose(fileptr);
    fileptr = NULL;




    struct timespec start, end;
    size_t p_size = sysconf(_SC_PAGE_SIZE);
    int pagesize = p_size;
    int pages;
    /*calloc allocates a separate block of memory for every page.*/
    void **pages_1 = (void **)calloc( pages, sizeof(void *));
    printf("\nFile size: %d bytes\n", filesize);
    printf("\nPage size: %d bytes\n", pagesize);

    //if and else statement to calculate number of pages
    if ((filesize/pagesize * pagesize) < filesize){
        pages = filesize/pagesize + 1;
    }
    else pages = filesize/pagesize;

    printf("\nNumber of pages: %d\n", pages);

    //Stores first byte of every page
    char *firstArray = (char *)malloc((pages)*sizeof(char));


    /*
     * For loop designed to write every byte retrieved from file into memory.
     * The memory being allocated for the bytes starts at the byteArray location.
     * mmap gives the allocated memory write and read privileges a page at a time.
     * memset writes the byte values into memory, one byte at a time.
     * madvise ensures allocated memory will be used with KSM
     */

    for(int i = 0; i < pages; i++){
        pages_1[i] = mmap(NULL, p_size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        //printf("\nOn Page: %d\n", i);
        for(int ii = 0; ii < p_size; ii++){
            if (ii == 0){
                firstArray[i] = byteArray[(i*p_size)+ii];
            }
            if (((i*p_size)+ii) < filesize ){
                //plus 8 used because memory stores the file bytes from opening the...
                //file as well as the exact same data stored in pages_1 location.
                //In other words KSM will combine pages_1 and file opened if not different.
                memset(pages_1[i] + ii, (unsigned) ((byteArray[(i*p_size)+ii]) + 8), 1);
                //used for debugging
                //printf(" %d ", byteArray[(i*p_size) + ii]);
            }
        }
        //essential for merging pages from VM and host.
        //this function does not need to be called in
        //VM levels.
        madvise(pages_1[i], p_size, MADV_MERGEABLE);
    }

    printf("\nUploaded file %s to memory....\n", argv[1]);



    //Additional wait time to ensure KSM has had enough time to work it's magic
    printf("\n%s has finished uploading. For enough time to possibly merge files, please wait 15 seconds.\n\n", argv[1]);
    sleep(15);
    printf("An adequate amount of time for merging has passed. \nPlease press \"q\" then enter to quit program. \n");


    /*
     * Recommended to wait at least 5-10 seconds after every press of enter
     */



    //Creates a time to refer to for unshared memory
    long unsharedWriteTime = 0;
    for (int r = 0; r < 5; r++){
        clock_gettime(CLOCK_REALTIME, &start);
        for(int i = 0; i < pages; i++){
           memset(pages_1[i],(unsigned)(firstArray[i] + 8),1);
        }
        clock_gettime(CLOCK_REALTIME, &end);
        //calculates average write time for unshared memory out of 5 writes
        unsharedWriteTime = (  (  ((end.tv_sec - start.tv_sec) * (1000000000)) + (end.tv_nsec - start.tv_nsec)  )  + unsharedWriteTime) / 2;
    }
    printf("Unshared write time: %d\n", unsharedWriteTime);
    printf("\nAutomatically looking for file merging...\n");


    //Below while loop keeps checking if fileWriteTime is far greater than
    //unsharedWriteTime, if so a message will be displayed.
    char press = NULL;
    long fileWriteTime;
    bool foundMerge = false;


    while(1){
        while(!foundMerge){
            sleep(5);
            clock_gettime(CLOCK_REALTIME, &start);
            for(int i = 0; i < pages; i++){
                //Writes the same data that was aready there at the beginning
                //of every page, ensuring a proper test every time enter is pressed
                memset(pages_1[i],(unsigned)(firstArray[i] + 8),1);
            }
            clock_gettime(CLOCK_REALTIME, &end);
            fileWriteTime = ((end.tv_sec - start.tv_sec) * (1000000000)) + (end.tv_nsec - start.tv_nsec);
            if (fileWriteTime > (unsharedWriteTime * 10)){
                printf("\nSHARED MEMORY FOR %s HAS BEEN DETECTED!\n", argv[1]);
                printf("\nFile write time for %s is %d, when it normally is %d\n",argv[1], fileWriteTime, unsharedWriteTime);
                foundMerge = true;
                printf("\nTo test again, press \"r\"\n");
            }
            if (press == 0x71){
                break;
            }

        }

        if (press == 'r'){
            foundMerge = false;
        }
        if (press == 0x71){
            break;
        }
        press = getchar();
    }

    //De-allocates memory
    free(pages_1);
    free(firstArray);
    free(byteArray);
    return 0;
}