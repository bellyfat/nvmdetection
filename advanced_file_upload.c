/*
 * Created by Taylor Roberts on 8/2/2019.
 *
 * Program uploads a file's byte information to memory.
 *
 * All of the code proved below is a piece of all the code in advanced_test_file_upload.c
 * Please refer to that file for notes.
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

    fileptr = fopen(argv[1], "rb");       // Open the file in binary mode
    if (fileptr == NULL){
        printf("\nCould not open file %s!\n\n", argv[1]);
        return 1;
    }
    fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
    filesize = ftell(fileptr);            // Get the current byte offset in the file
    rewind(fileptr);                      // Jump back to the beginning of the file
    byteArray = (char *)malloc((filesize)*sizeof(char));   // Enough memory for file + \0
    fread(byteArray, filesize, 1, fileptr);                // Read in the entire file
    rewind(fileptr);
    printf("\nFirst 10 bytes of file: ");
    for (int z = 0; z < 10; z++){
        printf(" %d ", (unsigned) byteArray[z]);
    }
    fclose(fileptr);
    fileptr = NULL;



    struct timespec start, end;
    size_t p_size = sysconf(_SC_PAGE_SIZE);
    int pagesize = p_size;
    int pages;
    void **pages_1 = (void **)calloc( pages, sizeof(void *));
    printf("\n\nFile size: %d bytes\n", filesize);
    printf("\nPage size: %d bytes\n", pagesize);
    if ((filesize/pagesize * pagesize) < filesize){
        pages = filesize/pagesize + 1;
    }
    else pages = filesize/pagesize;

    printf("\nNumber of pages: %d\n", pages);

    for(int i = 0; i < pages; i++){
        pages_1[i] = mmap(NULL, p_size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        for(int ii = 0; ii < p_size; ii++){
            if (((i*p_size)+ii) < filesize ){
                memset(pages_1[i] + ii, (unsigned) ((byteArray[(i*p_size)+ii]) + 8), 1);
            }
        }
    }

    printf("\nUploaded file %s to memory....\n", argv[1]);
    printf("\n%s has finished uploading. Please Wait 15 seconds.\n", argv[1]);
    sleep(15);
    printf("\nAn adequate amount of time for merging has passed. \nPlease press \"q\" then enter to quit program. \n");

    char press;

    while(1){
        if (press == 0x0a){
        }
        else if (press == 0x71){
            break;
        }
        press = getchar();
    }

    //De-allocates memory
    free(pages_1);
    free(byteArray);
    return 0;
}