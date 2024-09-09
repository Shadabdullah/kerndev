## Dynamic Circular Queue in Linux Character Device
This project is to implement a Circular_queue character device 

### Features

Dynamic Circular Queue: Queue size can be set dynamically using IOCTL commands.

Blocking Behavior
Push when Queue is Full: The process will block until space becomes available after a pop operation.
Pop when Queue is Empty: The process will block until data is pushed into the
Pop operation : pass data length to pop it will pop the data of provided length, if there is not enough data process will wait for enough data

User-Space Interaction: Example programs are provided to demonstrate the interaction between user space and the kernel device using IOCTL.

IOCTL Operations
SET_SIZE_OF_QUEUE: Set the size of the queue.
PUSH_DATA: Push data into the queue.
POP_DATA: Pop data from the queue.


program flow diagram :

## refer attached png for diagram
