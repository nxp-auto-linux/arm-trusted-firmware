/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <string.h>

#include <lib/utils_def.h>

#define ERROR(...) fprintf(stderr, "ERROR: " __VA_ARGS__)
#define VERBOSE(...) printf("VERBOSE: " __VA_ARGS__)
#define WARN(...) printf("WARN: " __VA_ARGS__)
#define INFO(...) printf("INFO: " __VA_ARGS__)

size_t strlcpy(char *dst, const char *src, size_t size);

#endif

