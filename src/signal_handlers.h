#ifndef GML_SIGNAL_HANDLERS_H
#define GML_SIGNAL_HANDLERS_H

#include <signal.h>

extern sig_atomic_t stopped;

void add_signal_handlers(void (*on_error)(void));

#endif