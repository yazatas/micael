typedef unsigned int size_t;

int write(int fd, char *buf, size_t size);
int execv(const char *path, const char *argv[]);

int main(int argc, char **argv)
{
    (void)argc, (void)argv;

    if (execv("/sbin/dsh", (void *)0) == -1)
        write(1, "failed to open /sbin/dsh", 24);
}
