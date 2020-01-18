typedef int pid_t;
typedef unsigned int size_t;

pid_t fork(void);
int read(int fd, char *buf, size_t size);
int write(int fd, char *buf, size_t size);
void _exit(int status);
int execv(const char *path, const char *argv[]);
pid_t wait(int *wstatus);

int main(int argc, char **argv)
{
    (void)argc, (void)argv;

    int ret         = 0;
    pid_t pid       = 0;
    char buffer[50] = { 0 };

    for (int i = 0; i < 5; ++i) {
        if ((pid = fork()) == 0) {
            ret = write(1, "HELLOOOOOOOOO\n", 14);
            for (;;) {
                asm volatile ("pause");
            }
        }
    }

    ret = write(1, "HELLO PARENTO\n", 14);
    _exit(0);

    for (;;);
}
