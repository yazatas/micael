typedef int pid_t;
typedef unsigned int size_t;

int read(int fd, char *buf, size_t size);
int write(int fd, char *buf, size_t size);

int main(int argc, char **argv)
{
    int ret         = 0;
    int status      = 0;
    pid_t pid       = 0;
    char buffer[50] = { 0 };

    while (1) {
        ret = write(1, "\nfirst number: ", 15);
        ret = read(0, buffer, 1);
        ret = write(1, "\nsecond number: ", 16);
        ret = read(0, buffer + 1, 1);
        ret = write(1, "\noperator: ", 11);
        ret = read(0, buffer + 2, 1);

        int fnum = buffer[0] - '0';
        int snum = buffer[1] - '0';
        int res  = 0;

        switch (buffer[2]) {
            case '+':
                res = fnum + snum;
                break;
            case '-':
                res = fnum - snum;
                break;
            case '*':
                res = fnum * snum;
                break;
        }

        if (res != 0) {
            if (res >= 10) {
                ret = 2;
                buffer[0] = (res / 10) + '0';
                buffer[1] = (res % 10) + '0';
            } else {
                ret = 1;
                buffer[0] = (res % 10) + '0';
            }

            write(1, "result: ", 8);
            write(1, buffer, ret);
        } else {
            write(1, "something went wrong", 20);
        }
    }
}
