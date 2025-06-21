#ifndef GML_METRIC_H
#define GML_METRIC_H

#include <stdio.h>
#include <math.h>

typedef struct metric {
	const char *name, *suffix;
	double count, sum, min, max;
} metric_t;

static void metric_init(metric_t* metric, const char* name, const char* suffix) {
	metric->name = name;
	metric->suffix = suffix;
	metric->count = 0;
	metric->sum = 0;
	metric->min = INFINITY;
	metric->max = 0;
}

static void metric_add(metric_t* metric, double value) {
    metric->count++;
    metric->sum += value;
    if (value < metric->min) metric->min = value;
    if (value > metric->max) metric->max = value;
}

static void metric_print(metric_t* metric) {
    printf("Average %1s: %3f %2s\n", metric->name, metric->suffix, metric->sum / metric->count);
    printf("Minimal %1s: %3f %2s\n", metric->name, metric->suffix, metric->min);
    printf("Maximal %1s: %3f %2s\n", metric->name, metric->suffix, metric->max);
}

#endif