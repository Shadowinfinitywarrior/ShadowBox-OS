#include "sys.h"
#include "errno.h"

static void print(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    sb_push(1, s, len);
}

#define SHM_SIZE 4096
#define IPC_CREAT 01000
#define IPC_RMID 0

#define SYS_SHMGET 252
#define SYS_SHMAT 253
#define SYS_SHMDT 254
#define SYS_SHMCTL 255

int main() {
	print("shm_test start\n");
    // Create a shared memory segment of one page
    uint64_t key = 0x1234;
    uint64_t size = SHM_SIZE; // defined as 4096 in ipc.h
    int shmflg = IPC_CREAT;
    int shmid = (int)syscall3(SYS_SHMGET, key, size, shmflg);
    if (shmid < 0) {
        return 1; // shmget failed
    }

    // Attach the segment
    void *addr = (void *)syscall3(SYS_SHMAT, (uint64_t)shmid, 0, 0);
    if ((uint64_t)addr == (uint64_t)-ENOENT) {
        return 2; // shmat failed
    }

    // Write a test value
    int *p = (int *)addr;
    *p = 0xdeadbeef;

    // Detach the segment
    int r = (int)syscall1(SYS_SHMDT, (uint64_t)addr);
    if (r != 0) {
        return 3; // shmdt failed
    }

    // Remove the segment
    r = (int)syscall3(SYS_SHMCTL, (uint64_t)shmid, IPC_RMID, 0);
    if (r != 0) {
        return 4; // shmctl failed
    }

    	print("shm_test done\n");
	return 0; // success
}
