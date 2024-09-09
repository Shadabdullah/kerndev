#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>

#define DRIVER_NAME "/dev/shad"
#define SET_SIZE_OF_QUEUE _IOW('a', 'a', int *)

int main(void) {
    int fd = open(DRIVER_NAME, O_RDWR);
    int size = 5;

    if (fd < 0) {
        perror("Failed to open the device, either no suce device or permission issues");
        return -1;
    }

    int ret = ioctl(fd, SET_SIZE_OF_QUEUE, &size);
    if (ret < 0) {
        perror("something went wrong couldn't set size of queue");
    }else{
    printf("Queue size set to : %d",size);
  }

    close(fd);
    return ret;
}
