#ifndef SYSCALL_TRACER_H
#define SYSCALL_TRACER_H

#include <stdint.h>

void syscall_tracer_init(void);
void syscall_tracer_log(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5);

#endif // SYSCALL_TRACER_H
