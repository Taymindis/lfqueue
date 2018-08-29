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
int max_queue_size = 1600;
lfqueue_t results;

lfqueue_init(&results, max_queue_size);

/** Wrap This scope in multithread testing **/
int_data = (int*) malloc(sizeof(int));
assert(int_data != NULL);
*int_data = i++;
/*Enqueue*/
 while (lfqueue_enq(&results, int_data) == -1) {
   	printf("ENQ Full ?\n");
}

/*Dequeue*/
while  ( (int_data = lfqueue_deq(&results)) == NULL) {
    printf("DEQ EMPTY ..\n");
}

// printf("%d\n", *(int*) ret );
free(ret);
/** End **/

lfqueue_destroy(&results);

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

## Testing 


## Uninstallation

```bash
cd build

sudo make uninstall

```


## You may also like lock free stack LIFO

[lfstack](https://github.com/Taymindis/lfstack)