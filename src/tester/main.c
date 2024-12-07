#include "../slobos.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/mman.h>

#include <unistd.h>

void *slobos_map(size_t sz)
{
    return mmap(NULL, sz, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
}
void slobos_unmap(void* blk, size_t sz)
{
    munmap(blk, sz);
}
size_t slobos_pgsize()
{
    static size_t cached = 0;
    if (!cached)
        cached = sysconf(_SC_PAGESIZE);
    return cached;
}

int main()
{
    slobos_allocator_t alloc = mmap(NULL, (slobos_allocator_size() + 0xfff) & ~0xfff, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    if (alloc == MAP_FAILED)
    {
        perror("mmap");
        return -1;
    }
    slobos_init(alloc, 0x1000, 0x4000);
    char* test = slobos_alloc(alloc, 15);
    memcpy(test, "Hello, world!\n", 14);
    printf("%s", test);
    return 0;
}
