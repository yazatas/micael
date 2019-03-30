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
    int ret         = 0;
    int status      = 0;
    pid_t pid       = 0;
    char buffer[50] = { 0 };

    while (1) {
        ret = write(1, "\nrficu@micael > ", 15);
        ret = read(0, buffer, 50);

        if ((pid = fork()) == 0) {
            buffer[ret] = '\0';

            if (execv(buffer, (void *)0) == -1) {
                write(1, "failed to open file\n", 20);
                _exit(1);
            }
        }
    }
}
