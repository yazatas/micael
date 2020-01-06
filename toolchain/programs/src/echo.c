typedef int pid_t;
typedef unsigned int size_t;

int read(int fd, char *buf, size_t size);
int write(int fd, char *buf, size_t size);
void _exit(int status);

int main(int argc, char **argv)
{
    (void)argc, (void)argv;

    int ret         = 0;
    char buffer[50] = { 0 };

    ret = write(1, "write something: \n", 17);
    ret = read(0, buffer, 50);
    ret = write(1, buffer, ret);
    _exit(0);
}
