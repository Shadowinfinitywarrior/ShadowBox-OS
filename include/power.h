#ifndef SHADOWBOX_POWER_H
#define SHADOWBOX_POWER_H

#include "types.h"

#define POWER_MAX_C_STATES 8
#define POWER_MAX_P_STATES 16
#define POWER_MAX_THERMAL_ZONES 8
#define POWER_NAME_MAX 32

#define C_STATE_NONE   0
#define C_STATE_C1     1
#define C_STATE_C2     2
#define C_STATE_C3     3
#define C_STATE_C4     4
#define C_STATE_C5     5
#define C_STATE_C6     6
#define C_STATE_C7     7

#define P_STATE_MAX_PERF  0
#define P_STATE_BALANCED  1
#define P_STATE_SAVE      2

#define POWER_EVENT_SUSPEND  0x01
#define POWER_EVENT_HIBERNATE 0x02
#define POWER_EVENT_SHUTDOWN 0x04
#define POWER_EVENT_LOW_BAT  0x08
#define POWER_EVENT_THERMAL  0x10

typedef struct acpi_fadt {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t preferred_pm_profile;
    uint16_t sci_int;
    uint32_t smi_cmd;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_cnt;
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_cnt_blk;
    uint32_t pm1b_cnt_blk;
    uint32_t pm2_cnt_blk;
    uint32_t pm_tmr_blk;
    uint32_t gpe0_blk;
    uint32_t gpe1_blk;
    uint8_t pm1_evt_len;
    uint8_t pm1_cnt_len;
    uint8_t pm2_cnt_len;
    uint8_t pm_tmr_len;
    uint8_t gpe0_blk_len;
    uint8_t gpe1_blk_len;
    uint8_t gpe1_base;
    uint8_t cst_cnt;
    uint16_t p_lvl2_lat;
    uint16_t p_lvl3_lat;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alrm;
    uint8_t mon_alrm;
    uint8_t century;
    uint16_t iapc_boot_arch;
    uint8_t reserved2;
    uint32_t flags;
} __attribute__((packed)) acpi_fadt_t;

typedef struct acpi_sdt_header {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) acpi_sdt_header_t;

typedef struct c_state {
    int type;
    char name[POWER_NAME_MAX];
    uint32_t latency;
    uint32_t power;
    uint64_t address;
    uint8_t address_space;
} c_state_t;

typedef struct p_state {
    int index;
    char name[POWER_NAME_MAX];
    uint32_t core_freq;
    uint32_t power;
    uint32_t transition_latency;
    uint32_t bus_master_latency;
    uint8_t control;
    uint8_t status;
} p_state_t;

typedef struct thermal_zone {
    int id;
    char name[POWER_NAME_MAX];
    int temperature;
    int critical_temp;
    int passive_temp;
    uint32_t polling_rate;
    int active_thresholds[4];
} thermal_zone_t;

typedef struct battery_status {
    int present;
    int charging;
    int level;
    int rate;
    int voltage;
    int capacity;
    char model[32];
    char vendor[16];
} battery_status_t;

void power_init(void);
void power_subsys_init(void);

int power_c_state_entry(int state);
int power_p_state_set(int index);
int power_get_c_state_count(void);
int power_get_p_state_count(void);
const c_state_t *power_get_c_state(int index);
const p_state_t *power_get_p_state(int index);

int power_thermal_init(void);
int power_thermal_read(int zone_id, int *temp);

int power_battery_get(battery_status_t *status);

void power_suspend(void);
int power_suspend_init(void);
int power_suspend_enter(void);
int power_suspend_exit(void);
void power_reboot(void);
void power_shutdown(void);

int power_profile_get(void);
void power_profile_set(int profile);

#endif
