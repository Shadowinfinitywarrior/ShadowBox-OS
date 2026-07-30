#ifndef SHADOWBOX_IPC_H
#define SHADOWBOX_IPC_H

#include "types.h"
#include "vfs.h"
#include "spinlock.h"
#include "task.h"

#define PIPE_BUF_SIZE 4096
#define MAX_PIPES 128
#define O_NONBLOCK 04000

#define IPC_CREAT  01000
#define IPC_EXCL   02000
#define IPC_NOWAIT 04000
#define IPC_RMID   0
#define IPC_STAT   1
#define IPC_SET    2

#define MAX_MSG_QUEUE 64

typedef struct pipe_buffer {
    uint8_t *data;
    uint32_t size;
    uint32_t capacity;
    uint32_t readers;
    uint32_t writers;
    uint32_t read_pos;
    uint32_t write_pos;
    spinlock_t lock;
    uint32_t flags;
} pipe_buffer_t;

typedef struct msg_buffer {
    long msg_type;
    uint32_t msg_size;
    struct msg_buffer *msg_next;
    uint8_t msg_data[];
} msg_buffer_t;

typedef struct msg_queue {
    uint32_t id;
    uint32_t key;
    uint32_t creator;
    uint32_t perms;
    struct msg_buffer *msg_head;
    struct msg_buffer *msg_tail;
    uint32_t msg_count;
    spinlock_t lock;
    struct msg_queue *next;
} msg_queue_t;

typedef struct sem_wait {
    struct process *proc;
    uint32_t needed;
    volatile int ready;
    struct sem_wait *next;
} sem_wait_t;

struct sembuf {
    unsigned short sem_num;
    short sem_op;
    short sem_flg;
};

typedef struct semaphore {
    uint32_t id;
    uint32_t key;
    uint32_t value;
    uint32_t max_value;
    uint32_t creator;
    uint32_t perms;
    spinlock_t lock;
    struct sem_wait *wait_head;
    struct sem_wait *wait_tail;
    struct semaphore *next;
} semaphore_t;

typedef struct shm_segment {
    uint32_t id;
    uint64_t size;
    uint64_t virt;
    uint64_t *pages;
    uint32_t refcount;
    uint32_t key;
    uint32_t creator;
    uint32_t perms;
    uint32_t page_count;
    struct shm_segment *next;
} shm_segment_t;

#define SHM_SIZE 4096
#define MAX_SHM_SEGMENTS 64

void ipc_init(void);
int pipe_create(vfs_node_t **read_end, vfs_node_t **write_end);
int pipe_read(pipe_buffer_t *pipe, void *buf, uint32_t size);
int pipe_write(pipe_buffer_t *pipe, const void *buf, uint32_t size);
void pipe_close(pipe_buffer_t *pipe, int is_writer);
int shmget(uint32_t key, uint64_t size, int flags);
void *shmat(int shmid, const void *shmaddr, int shmflg);
int shmdt(const void *shmaddr);
int shmctl(int shmid, int cmd, void *buf);
int msgget(uint32_t key, int flags);
int msgsnd(int msqid, const void *msgp, uint32_t msgsz, int msgflg);
int msgrcv(int msqid, void *msgp, uint32_t msgsz, long msgtyp, int msgflg);
int msgctl(int msqid, int cmd, void *buf);
int semget(uint32_t key, int nsems, int flags);
int semop(int semid, struct sembuf *sops, unsigned nsops);
int semctl(int semid, int semnum, int cmd, ...);

#endif
