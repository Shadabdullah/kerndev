#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define DRIVER_NAME "/dev/shad"
#define POP_DATA _IOR('a', 'c', struct data *)

struct data {
    int length;
    char *data;
};

int main(void) {
    int fd = open(DRIVER_NAME, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device");
        return -1;
    }

    struct data *d = malloc(sizeof(struct data));
    if (d == NULL) {
        perror("Failed to allocate memory for struct data");
        close(fd);
        return -1;
    }

    d->length = 2;  // We want to pop 3 bytes
    d->data = malloc(d->length + 1);  // +1 for null terminator
    if (d->data == NULL) {
        perror("Failed to allocate memory for data buffer");
        free(d);
        close(fd);
        return -1;
    }

    int ret = ioctl(fd, POP_DATA, d);
    if (ret < 0) {
            perror("Failed to pop data");
    } else {
        d->data[d->length] = '\0';  // Null-terminate the string
        printf("Popped data: %s\n", d->data);
    }

    close(fd);
    free(d->data);
    free(d);
    return ret < 0 ? -1 : 0;
}
