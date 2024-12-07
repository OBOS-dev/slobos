# slobos allocator
Some dumb allocator I wrote for obos that I made portable for some reason.
## Building the tester
### Prerequisites
- build-essentials (gcc, make, binutils)
### Build
Just run `make` in the cloned directory, and it should build the test runner.
## Porting
A header called `slobos_defines.h` must be in the include path.<br></br>
There are 4 macros that are required to be defined in this header:
- slobos_popcount
- slobos_bsf
- slobos_bsr
- SLOBOS_ALIGN

As well as one optional macro:
- CACHE_SIZE_DEFAULT
  
You must also define three functions:
```c
// Read-Write No-Execute
extern void  *slobos_map(size_t size);
extern void   slobos_unmap(void* base, size_t size);
extern size_t slobos_pgsize();
```
From there, you should be set to use the slobos allocator.
### Disclaimer
The provided allocator functions are *not* thread-safe by default, you must add locking on top of them.
### Usage
See [src/tester/main.c](src/tester/main.c) for an example of usage.
