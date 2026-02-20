#ifndef GML_SIGNAL_HANDLERS_H
#define GML_SIGNAL_HANDLERS_H

#include <signal.h>

extern volatile sig_atomic_t stopped;

/// @brief добавляет обработчик сигнала, который срабатывает при сигналах SIGILL, SIGABRT, SIGFPE, SIGSEGV.
/// При сигналах SIGINT, SIGTERM, SIGILL, SIGABRT, SIGFPE, SIGSEGV устанавливает флаг stopped = true.
/// Установка флага происходит до вызова указанного обработчика сигнала. При повторном вызове функции
/// add_error_signal_handler обработчик сигнала переназначается. Предыдущий сбрасывается. Обработчик может
/// быть вызван в любом из потоков процесса.
void add_error_signal_handler(void (*on_error)(void));

#endif
