#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>

/*int creat (const char *name, mode_t mode);
ssize_t write (int fd, const void *buf, size_t count); */

int main(int argc, char **argv)
{

    int fd;
    size_t wr_len;
    ssize_t nr;
    errno = 0;
    int txt_len = 0;

    printf("Writer started:\n");

    openlog(NULL, 0, LOG_USER);
    syslog(LOG_INFO, "Start Logging for writer");

    if (argc == 0)
    {
        printf("Error: Filename with path and file text not given.");
        syslog(LOG_ERR, "Invalid Number of Arguements: %d", argc);
        return 1;
    }
    else if (argc == 1)
    {
        printf("Error: Filename with path OR file text not given.");
        syslog(LOG_ERR, "Invalid Number of Arguements: %d", argc);
        return 1;
    }
    else
    {
        printf("Creating file \n");
        fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, S_IRWXU | S_IRWXG | S_IRWXO);
        if (fd == -1)
        {
            const int err = errno;
            fprintf(stderr, "File creation failed \n");
            syslog(LOG_ERR, "Error:%d", err);
            return 1;
        }
        else
        {
            printf("File creation success \n");

            txt_len = strlen(argv[2]);
            nr = write(fd, argv[2], txt_len);
            const int err = errno;
            if (nr == -1)
            {
                fprintf(stderr, "Write failed\n");
                syslog(LOG_ERR, "Error:%d", err);
                return 1;
            }
            else if (nr != txt_len)
            {
                fprintf(stderr, "Possible write failure, errno not set.\n");
                syslog(LOG_ERR, "Possible Error, please check");
            }
            else
            {
                printf("Write success\n");
            }
        }
    }
    close(fd);
    printf("Writer complete.\n");
    syslog(LOG_INFO, "Finished Logging for writer");
    closelog();
    return 0;
}
