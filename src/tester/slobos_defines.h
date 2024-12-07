#define slobos_popcount __builtin_popcount
#define slobos_bsf __builtin_ctzll
#define slobos_bsr __builtin_clzll

#define SLOBOS_ALIGN(align_to) __attribute__((aligned(align_to)));
