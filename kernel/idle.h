#ifndef SHADOWBOX_IDLE_H
#define SHADOWBOX_IDLE_H

// Initialise per‑CPU idle tasks. Must be called after SMP is initialised.
void idle_tasks_init(void);

#endif // SHADOWBOX_IDLE_H
