typedef int pid_t;
typedef unsigned int size_t;

int read(int fd, char *buf, size_t size);
int write(int fd, char *buf, size_t size);

int main(int argc, char **argv)
{
    (void)argc, (void)argv;

    int ret         = 0;
    char buffer[50] = { 0 };

    while (1) {
        ret = write(1, "\nwrite something", 15);
        ret = read(0, buffer, 50);
        ret = write(1, buffer, ret);
    }
}
