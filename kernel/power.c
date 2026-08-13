#include "power.h"
#include "kernel.h"
#include "acpi.h"
#include "vmm.h"
#include "pmm.h"
#include "pit.h"
#include "spinlock.h"
#include "kstring.h"
#include "io.h"

#define PHYS_TO_VIRT(phys) ((uint64_t)(phys) + 0xFFFF800000000000ULL)

static c_state_t c_states[POWER_MAX_C_STATES];
static int c_state_count;

static p_state_t p_states[POWER_MAX_P_STATES];
static int p_state_count;

static thermal_zone_t thermal_zones[POWER_MAX_THERMAL_ZONES];
static int thermal_zone_count;

static battery_status_t battery_status;

static int power_profile;
static int power_initialized;
static spinlock_t power_lock;

static uint32_t pm1a_cnt_blk;
static uint32_t pm1b_cnt_blk;

static uint8_t acpi_fadt_checksum(acpi_fadt_t *fadt) {
    uint8_t sum = 0;
    uint8_t *bytes = (uint8_t *)fadt;
    for (uint32_t i = 0; i < fadt->length; i++)
        sum += bytes[i];
    return sum;
}

static void parse_acpi_tables(void) {
    uint8_t *scan_start = (uint8_t *)0xFFFFFFFF800E0000;
    uint8_t *scan_end = (uint8_t *)0xFFFFFFFF80100000;
    acpi_fadt_t *fadt = NULL;

    for (uint8_t *ptr = scan_start; ptr < scan_end; ptr += 16) {
        if (ptr[0] == 'R' && ptr[1] == 'S' && ptr[2] == 'D' && ptr[3] == ' ' &&
            ptr[4] == 'P' && ptr[5] == 'T' && ptr[6] == 'R' && ptr[7] == ' ') {
            acpi_rsdp_t *rsdp = (acpi_rsdp_t *)ptr;
            uint32_t rsdt_phys = rsdp->rsdt_address;

            vmm_map_phys_range(rsdt_phys, 0x1000);

            acpi_sdt_header_t *rsdt = (acpi_sdt_header_t *)PHYS_TO_VIRT(rsdt_phys);
            uint32_t rsdt_len = rsdt->length;
            int entry_count = (rsdt_len - sizeof(acpi_sdt_header_t)) / 4;
            uint32_t *entry_ptr = (uint32_t *)((uint64_t)rsdt + sizeof(acpi_sdt_header_t));

            for (int i = 0; i < entry_count; i++) {
                uint64_t table_phys = entry_ptr[i];
                vmm_map_phys_range(table_phys, 0x1000);
                acpi_sdt_header_t *hdr = (acpi_sdt_header_t *)PHYS_TO_VIRT(table_phys);
                if (hdr->signature[0] == 'F' && hdr->signature[1] == 'A' &&
                    hdr->signature[2] == 'D' && hdr->signature[3] == 'T') {
                    fadt = (acpi_fadt_t *)hdr;
                    break;
                }
            }
            break;
        }
    }

    if (!fadt) {
        printk(KERN_DEBUG "Power: FADT not found, using defaults\n");
        return;
    }

    if (acpi_fadt_checksum(fadt) != 0) {
        printk(KERN_WARN "Power: FADT checksum invalid\n");
    }

    pm1a_cnt_blk = fadt->pm1a_cnt_blk;
    pm1b_cnt_blk = fadt->pm1b_cnt_blk;

    printk(KERN_DEBUG "Power: FADT found, PM1a cnt=0x%x, profile=0x%x\n",
           pm1a_cnt_blk, fadt->preferred_pm_profile);

    if (fadt->pm_tmr_blk) {
        uint32_t pm_tick = inl(fadt->pm_tmr_blk);
        printk(KERN_DEBUG "Power: ACPI PM timer tick=0x%x\n", pm_tick);
    }

    c_state_count = 0;
    c_states[c_state_count].type = C_STATE_C1;
    strncpy(c_states[c_state_count].name, "C1 (HLT)", sizeof(c_states[c_state_count].name));
    c_states[c_state_count].latency = 1;
    c_states[c_state_count].power = 1000;
    c_states[c_state_count].address = 0;
    c_state_count++;

    if (fadt->p_lvl2_lat <= 100) {
        c_states[c_state_count].type = C_STATE_C2;
        strncpy(c_states[c_state_count].name, "C2 (MWAIT)", sizeof(c_states[c_state_count].name));
        c_states[c_state_count].latency = fadt->p_lvl2_lat;
        c_states[c_state_count].power = 500;
        c_states[c_state_count].address = 0x10;
        c_state_count++;
    }

    if (fadt->p_lvl3_lat <= 1000) {
        c_states[c_state_count].type = C_STATE_C3;
        strncpy(c_states[c_state_count].name, "C3 (MWAIT)", sizeof(c_states[c_state_count].name));
        c_states[c_state_count].latency = fadt->p_lvl3_lat;
        c_states[c_state_count].power = 200;
        c_states[c_state_count].address = 0x20;
        c_state_count++;
    }

    p_state_count = 0;
    p_states[p_state_count].index = 0;
    p_states[p_state_count].core_freq = 0;
    p_states[p_state_count].power = 0;
    p_states[p_state_count].control = 0;
    p_states[p_state_count].status = 0;
    strncpy(p_states[p_state_count].name, "P0 (max perf)", sizeof(p_states[p_state_count].name));
    p_state_count++;

    p_states[p_state_count].index = 1;
    p_states[p_state_count].core_freq = 0;
    p_states[p_state_count].power = 0;
    p_states[p_state_count].control = 0x10;
    p_states[p_state_count].status = 0x10;
    strncpy(p_states[p_state_count].name, "P1 (balanced)", sizeof(p_states[p_state_count].name));
    p_state_count++;

    p_states[p_state_count].index = 2;
    p_states[p_state_count].core_freq = 0;
    p_states[p_state_count].power = 0;
    p_states[p_state_count].control = 0x20;
    p_states[p_state_count].status = 0x20;
    strncpy(p_states[p_state_count].name, "P2 (power save)", sizeof(p_states[p_state_count].name));
    p_state_count++;
}

void power_init(void) {
    spinlock_init(&power_lock);
    c_state_count = 0;
    p_state_count = 0;
    thermal_zone_count = 0;
    power_profile = P_STATE_BALANCED;
    strncpy(battery_status.model, "unknown", sizeof(battery_status.model));
    strncpy(battery_status.vendor, "unknown", sizeof(battery_status.vendor));
    battery_status.present = 0;
    battery_status.level = 100;
    battery_status.charging = 1;

    parse_acpi_tables();

    power_initialized = 1;
    printk(KERN_INFO "Power: Power management initialized (%d C-states, %d P-states)\n",
           c_state_count, p_state_count);
}

void power_subsys_init(void) {
    power_thermal_init();
}

int power_c_state_entry(int state) {
    if (!power_initialized || state < 0 || state >= c_state_count)
        return -1;

    switch (state) {
    case 0:
        __asm__ volatile("hlt");
        break;
    case 1:
        __asm__ volatile("mwait" :: "a"(0), "c"(0x10) : "memory");
        break;
    case 2:
        __asm__ volatile("mwait" :: "a"(0), "c"(0x20) : "memory");
        break;
    default:
        __asm__ volatile("hlt");
        break;
    }
    return 0;
}

int power_p_state_set(int index) {
    if (!power_initialized || index < 0 || index >= p_state_count)
        return -1;
    if (p_state_count > 0 && index < p_state_count) {
        power_profile = index;
    }
    return 0;
}

int power_get_c_state_count(void) {
    return c_state_count;
}

int power_get_p_state_count(void) {
    return p_state_count;
}

const c_state_t *power_get_c_state(int index) {
    if (index < 0 || index >= c_state_count) return NULL;
    return &c_states[index];
}

const p_state_t *power_get_p_state(int index) {
    if (index < 0 || index >= p_state_count) return NULL;
    return &p_states[index];
}

int power_thermal_init(void) {
    thermal_zone_count = 0;
    thermal_zones[thermal_zone_count].id = 0;
    strncpy(thermal_zones[thermal_zone_count].name, "CPU-thermal", sizeof(thermal_zones[thermal_zone_count].name));
    thermal_zones[thermal_zone_count].temperature = 40000;
    thermal_zones[thermal_zone_count].critical_temp = 100000;
    thermal_zones[thermal_zone_count].passive_temp = 85000;
    thermal_zones[thermal_zone_count].polling_rate = 1000;
    for (int i = 0; i < 4; i++)
        thermal_zones[thermal_zone_count].active_thresholds[i] = 0;
    thermal_zone_count++;

    printk(KERN_DEBUG "Power: Thermal subsystem initialized\n");
    return 0;
}

int power_thermal_read(int zone_id, int *temp) {
    if (!power_initialized || zone_id < 0 || zone_id >= thermal_zone_count || !temp)
        return -1;
    *temp = thermal_zones[zone_id].temperature;
    return 0;
}

int power_battery_get(battery_status_t *status) {
    if (!status) return -1;
    spin_lock(&power_lock);
    *status = battery_status;
    spin_unlock(&power_lock);
    return battery_status.present ? 0 : -1;
}

void power_suspend(void) {
    if (!power_initialized) return;
    printk(KERN_INFO "Power: System suspending...\n");
    if (pm1a_cnt_blk) {
        outw(pm1a_cnt_blk, 0x2000 | (1 << 13));
        __asm__ volatile("hlt");
    }
}

void power_reboot(void) {
    printk(KERN_INFO "Power: System rebooting...\n");
    uint8_t good = 0x02;
    while (good & 0x02)
        good = inb(0x64);
    outb(0x64, 0xFE);
    __asm__ volatile("hlt");
}

void power_shutdown(void) {
    printk(KERN_INFO "Power: System shutting down...\n");
    if (pm1a_cnt_blk) {
        outw(pm1a_cnt_blk, 0x2000);
    }
    while (1) {
        outw(0x604, 0x2000);
        outw(0x4004, 0x3400);
        __asm__ volatile("hlt");
    }
}

int power_profile_get(void) {
    return power_profile;
}

void power_profile_set(int profile) {
    if (profile >= P_STATE_MAX_PERF && profile <= P_STATE_SAVE)
        power_profile = profile;
}
