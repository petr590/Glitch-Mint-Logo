#ifndef GML_UTIL_RANDOM_H
#define GML_UTIL_RANDOM_H

#include <stdlib.h>

/** Возвращает рандомное число между from и to включительно.
 * Если to < from, то возвращает from. */
static inline int randrange(int from, int to) {
	return to < from ? from : rand() % (to - from + 1) + from;
}

/** Возвращает a или b случайно, с вероятностью 50/50. */
static inline int randchoose(int a, int b) {
	return (rand() & 0x1) ? a : b;
}

/**
 * Возвращает 1 с вероятностью chance, иначе 0.
 * @param chance число от 0 до 1.
 */
static inline int chance(float chance) {
	return (float) rand() / RAND_MAX < chance;
}

#endif