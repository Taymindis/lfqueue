# lfqueue
lock-free FIFO queue by C native built it, easy built cross platform(no extra dependencies needed) , guarantee thread safety memory management ever!


## API 
```c
int   lfqueue_init(lfqueue_t *lfqueue, size_t queue_size);
int   lfqueue_enq(lfqueue_t *lfqueue, void *value);
void *lfqueue_deq(lfqueue_t *lfqueue);
void lfqueue_destroy(lfqueue_t *lfqueue);
size_t lfqueue_size(lfqueue_t *lfqueue);
```


## Example

```c

int* ret;
int max_concurrent_thread = 16;
lfqueue_t my_queue;

lfqueue_init(&my_queue, max_concurrent_thread );

/** Wrap This scope in other threads **/
int_data = (int*) malloc(sizeof(int));
assert(int_data != NULL);
*int_data = i++;
/*Enqueue*/
 while (lfqueue_enq(&my_queue, int_data) == -1) {
    printf("ENQ Full ?\n");
}

/** Wrap This scope in other threads **/
/*Dequeue*/
while  ( (int_data = lfqueue_deq(&my_queue)) == NULL) {
    printf("DEQ EMPTY ..\n");
}

// printf("%d\n", *(int*) ret );
free(ret);
/** End **/

lfqueue_destroy(&my_queue);

```



## Build and Installation

For linux OS, you may use cmake build, for other platforms, please kindly include the source code and header file into the project, e.g. VS2017, DEV-C++, Xcode

```bash
mkdir build

cd build

cmake ..

make

./lfqueue-example

valgrind --tool=memcheck --leak-check=full ./lfqueue-example

sudo make install


```

## Endless Testing 

For endless testing until interrupt, if you having any issue while testing, please kindly raise an issue

```bash

./keep-testing.sh

```


## Uninstallation

```bash
cd build

sudo make uninstall

```


## You may also like lock free stack LIFO

[lfstack](https://github.com/Taymindis/lfstack)