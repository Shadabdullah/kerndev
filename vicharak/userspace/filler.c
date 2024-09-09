#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>

#define DRIVER_NAME "/dev/shad"
#define PUSH_DATA _IOW('a', 'b', struct data *)

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
    d->length = 3;
    d->data = malloc(3);
    memcpy(d->data, "xyz", 3);

    int ret = ioctl(fd, PUSH_DATA, d);
    if (ret < 0) {
        perror("couldn't push the data into queue");
    }else{

  printf(" %s data pushed successfully" ,d->data);
  }

    close(fd);
    free(d->data);
    free(d);
    return ret;
}
