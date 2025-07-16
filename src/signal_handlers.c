#include "signal_handlers.h"
#include <stdlib.h>
#include <stdbool.h>

volatile sig_atomic_t stopped = false;

static void set_stopped(int signum) {
	stopped = true;
}


static void (*on_error_func)(void);

static void wrap_on_error(int signum) {
	if (on_error_func) on_error_func();
}


void add_signal_handlers(void (*on_error)(void)) {
	on_error_func = on_error;

	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGILL);
	sigaddset(&mask, SIGABRT);
	sigaddset(&mask, SIGFPE);
	sigaddset(&mask, SIGSEGV);
	sigaddset(&mask, SIGTERM);
	
	struct sigaction normal_sigaction = {
		.sa_handler = set_stopped,
		.sa_mask = mask,
		.sa_flags = SA_RESTART,
		.sa_restorer = NULL,
	};
	
	sigaction(SIGINT,  &normal_sigaction, NULL);
	sigaction(SIGTERM, &normal_sigaction, NULL);
	

	struct sigaction error_action = {
		.sa_handler = wrap_on_error,
		.sa_mask = mask,
		.sa_flags = SA_RESTART,
		.sa_restorer = NULL,
	};
	
	sigaction(SIGILL,  &error_action, NULL);
	sigaction(SIGABRT, &error_action, NULL);
	sigaction(SIGFPE,  &error_action, NULL);
	sigaction(SIGSEGV, &error_action, NULL);
}