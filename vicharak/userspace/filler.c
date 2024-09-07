#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#define DRIVER_NAME "/dev/shad"
#define PUSH_DATA _IOW('a', 'b', struct data * )
struct data {
int length;
char * data;
};int main(void) {
int fd = open(DRIVER_NAME, O_RDWR);
struct data * d = malloc(sizeof(struct data));
d->length = 3;
d->data = malloc(3);
memcpy(d->data, "xyz", 3);
int ret = ioctl(fd, PUSH_DATA, d);
close(fd);
free(d->data);
free(d);
return ret;
}
