#ifndef SHADOWBOX_PARTITION_H
#define SHADOWBOX_PARTITION_H

#include "types.h"
#include "block.h"

// MBR Partition Entry
typedef struct {
    uint8_t  status;
    uint8_t  chs_first[3];
    uint8_t  type;
    uint8_t  chs_last[3];
    uint32_t lba_start;
    uint32_t sector_count;
} __attribute__((packed)) mbr_partition_t;

// GPT GUID
typedef struct {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];
} __attribute__((packed)) gpt_guid_t;

// GPT Partition Entry
typedef struct {
    gpt_guid_t type_guid;
    gpt_guid_t partition_guid;
    uint64_t   lba_start;
    uint64_t   lba_end;
    uint64_t   attributes;
    uint16_t   name[36]; // UTF-16
} __attribute__((packed)) gpt_partition_t;

// Block Subsystem Hooks
void partition_scan_device(block_device_t *dev);

#endif
