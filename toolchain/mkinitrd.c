#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* how to use this:
 *
 * gcc -o mkinitrd mkinitrd.c && ./mkinitrd <file1> <file2> ... <file4> 
 *
 * At this moment, directories are not supported so this program will generate 
 * a disk file that contains an initrd header followed by a list of files */

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

    FILE *disk_fp = fopen("initrd.bin", "w"), *tmp;
    char *buffer  = NULL;

    disk_header_t disk_header = {
        .file_count = argc - 1,
        .disk_size  = sizeof(disk_header_t),
        .magic      = HEADER_MAGIC
    };

    file_header_t file = {
        .file_name = "",
        .file_size = 0,
        .magic     = FILE_MAGIC
    };
    
    /* header must be written after files because
     * header info depends on file size data
     *
     * skip sizeof(disk_header_t) bytes from 
     * the beginning of disk to save space for header */
    fseek(disk_fp, sizeof(disk_header_t), SEEK_CUR);

    for (int i = 1; i < argc; ++i) {
        tmp = fopen(argv[i], "r");
        
        file.file_size = get_file_size(tmp);
        strncpy(file.file_name, argv[i], FNAME_MAXLEN);

        buffer = malloc(file.file_size);
        printf("file size: %u bytes\n", file.file_size);

        fread(buffer, 1, file.file_size, tmp);

        fwrite(&file,  sizeof(file_header_t), 1, disk_fp);
        fwrite(buffer, 1, file.file_size, disk_fp);
        fclose(tmp);

        disk_header.disk_size += file.file_size + sizeof(file_header_t);
    }

    fseek(disk_fp, 0L, SEEK_SET);
    fwrite(&disk_header, sizeof(disk_header_t), 1, disk_fp);

    printf("disk size: %u bytes\n", disk_header.disk_size);

    fclose(disk_fp);
    return 0;
}
