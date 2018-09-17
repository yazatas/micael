#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* how to use this:
 *
 * gcc -o verify verify_mkinitrd.c && ./verify <file1> <file2> ... <file4> 
 *
 * This program shall verify that the generated initrd is valid.
 * initrd verification is used only for debugging */

#define HEADER_MAGIC  0x13371338
#define FILE_MAGIC    0xdeadbeef
#define FNAME_MAXLEN  64

typedef struct disk_header {
    uint8_t  file_count;
    uint32_t disk_size;
    uint32_t crc32;
    uint32_t magic;
} disk_header_t;

typedef struct file_header {
    char file_name[FNAME_MAXLEN];
    uint32_t file_size;
    uint32_t crc32;
    uint32_t magic;
} file_header_t;

uint32_t get_file_size(FILE *fp)
{
    uint32_t fsize = 0;

    fseek(fp, 0L, SEEK_END);
    fsize = (uint32_t)ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    return fsize;
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        return -1;
    }

    FILE *disk_fp = fopen("initrd.bin", "r"), *tmp;
    char *buffer  = NULL;

    disk_header_t disk_header;
    file_header_t file_header;

    fread(&disk_header, sizeof(disk_header_t), 1, disk_fp);

    if (disk_header.magic != HEADER_MAGIC) {
        fprintf(stderr, "invalid magic number for disk header!\n");
        return -1;
    }

    fprintf(stderr, "file count: %u\ndisk size:  %u\n", 
            disk_header.file_count, disk_header.disk_size);

    fseek(disk_fp, sizeof(disk_header_t), SEEK_SET);

    for (int i = 1; i < argc; ++i) {
        fread(&file_header, sizeof(file_header_t), 1, disk_fp);

        if (file_header.magic != FILE_MAGIC) {
            fprintf(stderr, "invalid magic number for file header!\n");
            return -1;
        }

        fprintf(stderr, "file info:\n\
                \tname:  %s\n\
                \tsize:  %u\n\
                \tmagic: 0x%x\n\n", file_header.file_name, 
                                    file_header.file_size, 
                                    file_header.magic);

        FILE *verify_fp  = fopen(argv[i], "r");
        char *initrd_buf = malloc(file_header.file_size);
        char *verify_buf = malloc(get_file_size(verify_fp));

        fread(initrd_buf, file_header.file_size, 1, disk_fp);
        fread(verify_buf, file_header.file_size, 1, verify_fp);

        for (uint32_t k = 0; k < file_header.file_size; ++k) {
            if (initrd_buf[k] != verify_buf[k]) {
                fprintf(stderr, "byte at index %u does NOT match!\n", k);
                return -1;
            }
        }

        fprintf(stderr, "file at index %u (size %u) OK!\n\n", i, file_header.file_size);

        fclose(verify_fp);
        free(initrd_buf);
        free(verify_buf);
    }
        
    fclose(disk_fp);
    return 0;
}
