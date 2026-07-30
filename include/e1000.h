#ifndef E1000_H
#define E1000_H

#include "types.h"
#include "pci.h"

#define E1000_VENDOR_ID 0x8086
#define E1000_DEVICE_ID 0x100E

#define NUM_TX_DESC 64
#define NUM_RX_DESC 128
#define RX_BUF_SIZE 2048

/* Register map (offsets from MMIO base) */
#define E1000_CTRL    0x0000
#define E1000_STATUS  0x0008
#define E1000_EECD    0x0010
#define E1000_ICR     0x00C0
#define E1000_ITR     0x00C4
#define E1000_ICS     0x00C8
#define E1000_IMS     0x00D0
#define E1000_IMC     0x00D8
#define E1000_RCTL    0x0100
#define E1000_TCTL    0x0400
#define E1000_TIPG    0x0410
#define E1000_RDBAL   0x2800
#define E1000_RDBAH   0x2804
#define E1000_RDLEN   0x2808
#define E1000_RDH     0x2810
#define E1000_RDT     0x2818
#define E1000_RDTR    0x2820
#define E1000_RADV    0x282C
#define E1000_RSRPD   0x2C00
#define E1000_TDBAL   0x3800
#define E1000_TDBAH   0x3804
#define E1000_TDLEN   0x3808
#define E1000_TDH     0x3810
#define E1000_TDT     0x3818
#define E1000_MTA     0x5200
#define E1000_RAL     0x5400
#define E1000_RAH     0x5404

/* CTRL bits */
#define E1000_CTRL_FD       0x00000001
#define E1000_CTRL_LRST     0x00000008
#define E1000_CTRL_ASDE     0x00000020
#define E1000_CTRL_SLU      0x00000040
#define E1000_CTRL_ILOS     0x00000080
#define E1000_CTRL_RST      0x04000000

/* STATUS bits */
#define E1000_STATUS_LU     0x00000002

/* RCTL bits */
#define E1000_RCTL_EN       0x00000002
#define E1000_RCTL_SBP      0x00000004
#define E1000_RCTL_UPE      0x00000008
#define E1000_RCTL_MPE      0x00000010
#define E1000_RCTL_LPE      0x00000020
#define E1000_RCTL_BAM      0x00008000
#define E1000_RCTL_SECRC    0x04000000
#define E1000_RCTL_BSIZE_2K 0x00000000
#define E1000_RCTL_BSIZE_4K 0x00030000

/* TCTL bits */
#define E1000_TCTL_EN       0x00000002
#define E1000_TCTL_PSP      0x00000008
#define E1000_TCTL_CT_SHIFT 4
#define E1000_TCTL_COLD_SHIFT 12
#define E1000_TCTL_CT       0x00000FF0
#define E1000_TCTL_COLD     0x003FF000

/* TX descriptor CMD bits */
#define E1000_TXD_CMD_EOP   0x01
#define E1000_TXD_CMD_IFCS  0x02
#define E1000_TXD_CMD_RS    0x08

/* TX descriptor STATUS bits */
#define E1000_TXD_STAT_DD   0x01

/* RX descriptor STATUS bits */
#define E1000_RXD_STAT_DD   0x01
#define E1000_RXD_STAT_EOP  0x02

/* Interrupt bits */
#define E1000_ICR_TXDW      0x00000001
#define E1000_ICR_TXQE      0x00000002
#define E1000_ICR_LSC       0x00000004
#define E1000_ICR_RXSEQ     0x00000008
#define E1000_ICR_RXDMT0    0x00000010
#define E1000_ICR_RXO       0x00000040
#define E1000_ICR_RXT0      0x00000080
#define E1000_ICR_TXOC      0x00400000

struct e1000_tx_desc {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} __attribute__((packed));

struct e1000_rx_desc {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed));

void e1000_init(pci_device_t *pci_dev);

#endif
