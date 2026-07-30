#ifndef SHADOWBOX_IO_H
#define SHADOWBOX_IO_H

#include "types.h"

/*
 * outb - Write a byte to an I/O port
 * @port: Port address
 * @val: Value to write
 */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * inb - Read a byte from an I/O port
 * @port: Port address
 * Returns: Value read
 */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*
 * outw - Write a 16-bit word to an I/O port
 * @port: Port address
 * @val: Value to write
 */
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * inw - Read a 16-bit word from an I/O port
 * @port: Port address
 * Returns: Value read
 */
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*
 * outl - Write a 32-bit dword to an I/O port
 * @port: Port address
 * @val: Value to write
 */
static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * inl - Read a 32-bit dword from an I/O port
 * @port: Port address
 * Returns: Value read
 */
static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#endif
