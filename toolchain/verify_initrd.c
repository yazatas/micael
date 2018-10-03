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
#define DIR_MAGIC     0xcafebabe

#define NAME_MAXLEN   64
#define MAX_FILES     5
#define MAX_DIRS      5

typedef struct disk_header {
    uint8_t  file_count;
    uint32_t disk_size;
    uint32_t magic;
} disk_header_t;

typedef struct dir_header {
    char dir_name[NAME_MAXLEN];
    uint32_t item_count;

    /* how many bytes the directory items take in total 
     * Useful for skipping directories */
    uint32_t bytes_len;

    uint32_t inode;
    uint32_t magic;
} dir_header_t;

typedef struct file_header {
    char file_name[NAME_MAXLEN];
    uint32_t file_size;
    uint32_t inode;
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
    FILE *disk_fp = fopen("initrd.bin", "r"), *tmp;

    disk_header_t disk_header;
    dir_header_t  root_header;

    uint8_t  file_count;
    uint32_t disk_size;
    uint32_t magic;

    fread(&disk_header, sizeof(disk_header_t), 1, disk_fp);

    printf("file_count: %u\n"
           "disk_size:  %u\n"
           "magic:      0x%x\n", disk_header.file_count,
           disk_header.disk_size, disk_header.magic);

    puts("\n------------");

    fread(&root_header, sizeof(dir_header_t), 1, disk_fp);

    printf("name:       %s\n"
           "item count: %u\n"
           "num bytes:  %u\n"
           "inode:      %u\n"
           "\ndirectory contents:\n",
           root_header.dir_name,  root_header.item_count,
           root_header.bytes_len, root_header.inode);

    dir_header_t dir;
    file_header_t file;

    for (int dir_iter = 0; dir_iter < root_header.item_count; ++dir_iter) {
        fread(&dir, sizeof(dir_header_t), 1, disk_fp);

        printf("name:       %s\n"
               "item count: %u\n"
               "num bytes:  %u\n"
               "inode:      %u\n"
               "\ndirectory contents:\n",
               dir.dir_name,  dir.item_count,
               dir.bytes_len, dir.inode);

        for (int file_iter = 0; file_iter < dir.item_count; ++file_iter) {
            fread(&file, sizeof(file_header_t), 1, disk_fp);
            printf("\tname:  %s\n"
                   "\tsize:  %u\n"
                   "\tinode: %u\n\n",
                file.file_name, file.file_size,
                file.inode, file.magic);
            fseek(disk_fp, file.file_size, SEEK_CUR);
        }
    }

    return 0;
}
