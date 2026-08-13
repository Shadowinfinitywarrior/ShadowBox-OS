#ifndef SHADOWBOX_NVME_H
#define SHADOWBOX_NVME_H

#include "types.h"
#include "device.h"
#include "block.h"

// NVMe Submission/Completion Queue Entry sizes
#define NVME_SQE_SIZE 64
#define NVME_CQE_SIZE 16

typedef struct nvme_sq_entry {
    uint8_t opcode;
    uint8_t flags;
    uint16_t command_id;
    uint32_t nsid;
    uint64_t reserved;
    uint64_t metadata;
    uint64_t prp1;
    uint64_t prp2;
    uint32_t cdw10, cdw11, cdw12, cdw13, cdw14, cdw15;
} __attribute__((packed)) nvme_sq_entry_t;

typedef struct nvme_cq_entry {
    uint32_t cdw0;
    uint32_t reserved;
    uint16_t sq_head;
    uint16_t sq_id;
    uint16_t command_id;
    uint16_t status;
} __attribute__((packed)) nvme_cq_entry_t;

void nvme_init(void);

#endif
