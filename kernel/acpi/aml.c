#include <stddef.h>
#include <stdint.h>
#include "acpi.h"

/* Minimal stub implementation for ACPI AML handling */

typedef struct {
    /* Placeholder for internal AML parsing context */
    void *private;
} aml_context_t;

/* Initialize AML subsystem – currently a no‑op placeholder */
void aml_init(void) {
    /* No initialization required for stub */
}

/* Parse AML bytecode – returns 0 on success, does nothing */
int aml_parse(const uint8_t *code, size_t len, aml_context_t *ctx) {
    (void)code;
    (void)len;
    (void)ctx;
    return 0; /* Success */
}

/* Execute an AML method – placeholder implementation */
int aml_execute(aml_context_t *ctx) {
    (void)ctx;
    return 0; /* Success */
}
