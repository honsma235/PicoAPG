/*
 * Core1 entry point wrapper
 */

#ifndef MAIN_CORE1_H
#define MAIN_CORE1_H

#ifdef __cplusplus
extern "C" {
#endif

void abort_all(void);

void reset_to_defaults_all(void);

void main_core1_entry(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_CORE1_H */