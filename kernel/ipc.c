#include "ipc.h"
#include "kernel.h"
#include "vfs.h"
#include "malloc.h"
#include "kstring.h"
#include "task.h"
#include "errno.h"
#include "pmm.h"
#include "vmm.h"

static struct shm_segment *shm_list = 0;
static uint32_t next_shm_id = 1;
static struct msg_queue *msg_queue_list = 0;
static uint32_t next_msg_id = 1;
static struct semaphore *sem_list = 0;
static uint32_t next_sem_id = 1;

void ipc_init(void) {
    printk("IPC: Initializing Inter-Process Communication subsystem...\n");
}

int pipe_create(vfs_node_t **read_end, vfs_node_t **write_end) {
    pipe_buffer_t *p = kmalloc(sizeof(pipe_buffer_t));
    if (!p) return -ENOMEM;
    memset(p, 0, sizeof(pipe_buffer_t));
    p->data = kmalloc(PIPE_BUF_SIZE);
    if (!p->data) { kfree(p); return -ENOMEM; }
    p->capacity = PIPE_BUF_SIZE;
    p->readers = 1;
    p->writers = 1;
    p->read_pos = 0;
    p->write_pos = 0;
    spinlock_init(&p->lock);

    vfs_node_t *rnode = kmalloc(sizeof(vfs_node_t));
    vfs_node_t *wnode = kmalloc(sizeof(vfs_node_t));
    if (!rnode || !wnode) { kfree(p->data); kfree(p); kfree(rnode); kfree(wnode); return -ENOMEM; }
    memset(rnode, 0, sizeof(vfs_node_t));
    memset(wnode, 0, sizeof(vfs_node_t));
    rnode->flags = FS_PIPE;
    wnode->flags = FS_PIPE;
    rnode->impl = (uint64_t)p;
    wnode->impl = (uint64_t)p;
    rnode->read = (read_type_t)pipe_read;
    wnode->write = (write_type_t)pipe_write;
    *read_end = rnode;
    *write_end = wnode;
    return 0;
}

int pipe_read(pipe_buffer_t *pipe, void *buf, uint32_t size) {
    if (!pipe || !buf) return -EINVAL;
    spin_lock_irqsave(&pipe->lock);
    uint32_t read = 0;
    while (read < size) {
        if (pipe->read_pos != pipe->write_pos) {
            ((uint8_t *)buf)[read++] = pipe->data[pipe->read_pos++];
            pipe->read_pos %= pipe->capacity;
        } else break;
    }
    spin_unlock_irqrestore(&pipe->lock);
    return read;
}

int pipe_write(pipe_buffer_t *pipe, const void *buf, uint32_t size) {
    if (!pipe || !buf) return -EINVAL;
    spin_lock_irqsave(&pipe->lock);
    uint32_t written = 0;
    while (written < size) {
        uint32_t next = (pipe->write_pos + 1) % pipe->capacity;
        if (next != pipe->read_pos) {
            pipe->data[pipe->write_pos] = ((const uint8_t *)buf)[written++];
            pipe->write_pos = next;
        } else break;
    }
    spin_unlock_irqrestore(&pipe->lock);
    return written;
}

void pipe_close(pipe_buffer_t *pipe, int is_writer) {
    if (!pipe) return;
    spin_lock_irqsave(&pipe->lock);
    if (is_writer) pipe->writers--;
    else pipe->readers--;
    int should_free = (pipe->readers == 0 && pipe->writers == 0);
    spin_unlock_irqrestore(&pipe->lock);
    if (should_free) {
        if (pipe->data) kfree(pipe->data);
        kfree(pipe);
    }
}

int shmget(uint32_t key, uint64_t size, int flags) {
    (void)flags;
    struct shm_segment *s = shm_list;
    while (s) { if (s->key == key) return s->id; s = s->next; }

    struct shm_segment *seg = kmalloc(sizeof(struct shm_segment));
    if (!seg) return -ENOMEM;
    memset(seg, 0, sizeof(struct shm_segment));
    seg->id = next_shm_id++;
    seg->key = key;
    seg->size = size;
    seg->refcount = 0;
    seg->creator = get_current_process()->pid;
    seg->next = shm_list;
    shm_list = seg;
    return seg->id;
}

void *shmat(int shmid, const void *shmaddr, int shmflg) {
    (void)shmaddr; (void)shmflg;
    struct shm_segment *s = shm_list;
    while (s) {
        if (s->id != (uint32_t)shmid) { s = s->next; continue; }

        if (!s->pages) {
            uint64_t pages = (s->size + 4095) / 4096;
            s->pages = kmalloc(pages * sizeof(uint64_t));
            struct process *proc = get_current_process();
            uint64_t old_cr3;
            __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
            if (proc->cr3 != old_cr3)
                __asm__ volatile("mov %0, %%cr3" :: "r"(proc->cr3) : "memory");

            s->virt = aslr_get_mmap_base();
            for (uint64_t i = 0; i < pages; i++) {
                s->pages[i] = (uint64_t)pmm_alloc_page();
                vmm_map_page(s->pages[i], s->virt + i * 4096,
                            PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
            }
            if (proc->cr3 != old_cr3)
                __asm__ volatile("mov %0, %%cr3" :: "r"(old_cr3) : "memory");
        }
        s->refcount++;
        return (void *)s->virt;
    }
    return (void *)-ENOENT;
}

int shmdt(const void *shmaddr) { (void)shmaddr; return 0; }
int shmctl(int shmid, int cmd, void *buf) { (void)shmid; (void)cmd; (void)buf; return 0; }

static struct msg_queue *msgq_find(uint32_t key) {
    struct msg_queue *q = msg_queue_list;
    while (q) { if (q->key == key) return q; q = q->next; }
    return 0;
}

int msgget(uint32_t key, int flags) {
    (void)flags;
    struct msg_queue *q = msgq_find(key);
    if (q) return q->id;

    struct msg_queue *mq = kmalloc(sizeof(struct msg_queue));
    if (!mq) return -ENOMEM;
    memset(mq, 0, sizeof(struct msg_queue));
    mq->id = next_msg_id++;
    mq->key = key;
    mq->creator = get_current_process()->pid;
    mq->msg_count = 0;
    mq->msg_head = 0;
    mq->msg_tail = 0;
    spinlock_init(&mq->lock);
    mq->next = msg_queue_list;
    msg_queue_list = mq;
    return mq->id;
}

int msgsnd(int msqid, const void *msgp, uint32_t msgsz, int msgflg) {
    struct msg_queue *q = msg_queue_list;
    while (q) { if (q->id == (uint32_t)msqid) break; q = q->next; }
    if (!q) return -EINVAL;

    struct msg_buffer *buf = kmalloc(sizeof(struct msg_buffer) + msgsz);
    if (!buf) return -ENOMEM;
    memcpy(buf, msgp, sizeof(long));
    buf->msg_size = msgsz;
    memcpy(buf->msg_data, (const uint8_t *)msgp + sizeof(long), msgsz);

    spin_lock_irqsave(&q->lock);
    if (q->msg_count >= MAX_MSG_QUEUE) {
        spin_unlock_irqrestore(&q->lock);
        if (msgflg & IPC_NOWAIT) { kfree(buf); return -EAGAIN; }
        while (q->msg_count >= MAX_MSG_QUEUE) {
            spin_unlock_irqrestore(&q->lock);
            yield();
            spin_lock_irqsave(&q->lock);
        }
    }
    if (q->msg_count == 0) {
        q->msg_head = buf;
        q->msg_tail = buf;
    } else {
        q->msg_tail->msg_next = buf;
        q->msg_tail = buf;
    }
    buf->msg_next = 0;
    q->msg_count++;
    spin_unlock_irqrestore(&q->lock);
    return msgsz;
}

int msgrcv(int msqid, void *msgp, uint32_t msgsz, long msgtyp, int msgflg) {
    (void)msgtyp;
    struct msg_queue *q = msg_queue_list;
    while (q) { if (q->id == (uint32_t)msqid) break; q = q->next; }
    if (!q) return -EINVAL;

    spin_lock_irqsave(&q->lock);
    if (q->msg_count == 0) {
        spin_unlock_irqrestore(&q->lock);
        if (msgflg & IPC_NOWAIT) return -ENOMSG;
        while (q->msg_count == 0) {
            yield();
            spin_lock_irqsave(&q->lock);
        }
    }
    struct msg_buffer *buf = q->msg_head;
    q->msg_head = buf->msg_next;
    if (!q->msg_head) q->msg_tail = 0;
    q->msg_count--;
    spin_unlock_irqrestore(&q->lock);

    uint32_t copy_sz = (buf->msg_size < msgsz) ? buf->msg_size : msgsz;
    memcpy(msgp, &buf->msg_type, sizeof(long));
    memcpy((uint8_t *)msgp + sizeof(long), buf->msg_data, copy_sz);
    int ret = copy_sz;
    kfree(buf);
    return ret;
}

int msgctl(int msqid, int cmd, void *buf) {
    (void)msqid; (void)cmd; (void)buf;
    return 0;
}

static struct semaphore *sem_find(uint32_t key) {
    struct semaphore *s = sem_list;
    while (s) { if (s->key == key) return s; s = s->next; }
    return 0;
}

int semget(uint32_t key, int nsems, int flags) {
    (void)nsems; (void)flags;
    struct semaphore *s = sem_find(key);
    if (s) return s->id;

    struct semaphore *sem = kmalloc(sizeof(struct semaphore));
    if (!sem) return -ENOMEM;
    memset(sem, 0, sizeof(struct semaphore));
    sem->id = next_sem_id++;
    sem->key = key;
    sem->value = (nsems > 0) ? nsems : 1;
    sem->max_value = sem->value;
    sem->creator = get_current_process()->pid;
    spinlock_init(&sem->lock);
    sem->wait_head = 0;
    sem->wait_tail = 0;
    sem->next = sem_list;
    sem_list = sem;
    return sem->id;
}

int semop(int semid, struct sembuf *sops, unsigned nsops) {
    struct semaphore *sem = sem_list;
    while (sem) { if (sem->id == (uint32_t)semid) break; sem = sem->next; }
    if (!sem) return -EINVAL;

    for (unsigned i = 0; i < nsops; i++) {
        spin_lock_irqsave(&sem->lock);
        if (sops[i].sem_op > 0) {
            sem->value += sops[i].sem_op;
            if (sem->value > sem->max_value) sem->max_value = sem->value;
            struct sem_wait *w = sem->wait_head;
            while (w) {
                if (sem->value >= w->needed) {
                    w->ready = 1;
                    w->proc->state = TASK_READY;
                    sched_enqueue(w->proc);
                    struct sem_wait *next = w->next;
                    w->next = 0;
                    w = next;
                } else break;
                w = w->next;
            }
        } else if (sops[i].sem_op < 0) {
            int needed = -sops[i].sem_op;
            while (sem->value < (uint32_t)needed) {
                struct sem_wait *w = kmalloc(sizeof(struct sem_wait));
                w->proc = get_current_process();
                w->needed = needed;
                w->ready = 0;
                w->next = 0;
                if (sem->wait_tail) sem->wait_tail->next = w;
                else sem->wait_head = w;
                sem->wait_tail = w;
                get_current_process()->state = TASK_BLOCKED;
                spin_unlock_irqrestore(&sem->lock);
                yield();
                spin_lock_irqsave(&sem->lock);
            }
            sem->value -= needed;
        }
        spin_unlock_irqrestore(&sem->lock);
    }
    return 0;
}

int semctl(int semid, int semnum, int cmd, ...) {
    (void)semid; (void)semnum; (void)cmd;
    return 0;
}
