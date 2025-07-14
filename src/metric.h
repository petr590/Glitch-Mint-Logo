#ifndef GML_METRIC_H
#define GML_METRIC_H

#include <stdio.h>
#include <math.h>

typedef struct metric {
	const char *name, *suffix;
	double sum, min, max;
	int count;
} metric_t;

#define METRIC_INITIALIZER(_name, _suffix) { .name = _name, .suffix = _suffix, .sum = 0, .min = INFINITY, .max = 0, .count = 0 }

static void metric_add(metric_t* metric, double value) {
    metric->count++;
    metric->sum += value;
    if (value < metric->min) metric->min = value;
    if (value > metric->max) metric->max = value;
}

static void metric_print(metric_t* metric) {
	double average = metric->sum / metric->count;
    printf("Average %s: %6.2f %s\n", metric->name, average,     metric->suffix);
    printf("Minimal %s: %6.2f %s\n", metric->name, metric->min, metric->suffix);
    printf("Maximal %s: %6.2f %s\n", metric->name, metric->max, metric->suffix);
}

#endif