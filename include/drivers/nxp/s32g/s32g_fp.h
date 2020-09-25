// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020 NXP
 */
#ifndef S32G_FP_H
#define S32G_FP_H

#define FP_PRECISION 100000000U

struct fp_data {
	uint64_t val;
};

static inline struct fp_data u2fp(uint64_t val)
{
	struct fp_data res = {
		.val = val * FP_PRECISION,
	};

	return res;
}

static inline uint64_t fp2u(struct fp_data val)
{
	return val.val / FP_PRECISION;
}

static inline void __reduce_factors(uint64_t *a, uint64_t *b)
{
	while (*a % 10 == 0 && *b % 10 == 0) {
		*a /= 10;
		*b /= 10;
	}
}

static inline struct fp_data fp_div(struct fp_data a, struct fp_data b)
{
	struct fp_data res;
	uint64_t div_factor = FP_PRECISION;

	/* Avoid overflow if possible */
	__reduce_factors(&a.val, &b.val);
	__reduce_factors(&div_factor, &b.val);

	res.val = a.val * div_factor / b.val;

	return res;
}

static inline struct fp_data fp_mul(struct fp_data a, struct fp_data b)
{
	uint64_t factor = FP_PRECISION;
	struct fp_data res;

	/* Avoid overflow if possible */
	__reduce_factors(&a.val, &factor);
	__reduce_factors(&b.val, &factor);

	res.val = a.val * b.val / factor;

	return res;
}

static inline struct fp_data fp_add(struct fp_data a, struct fp_data b)
{
	struct fp_data res = {
		.val = a.val + b.val,
	};

	return res;
}

static inline struct fp_data fp_sub(struct fp_data a, struct fp_data b)
{
	struct fp_data res = {
		.val = a.val - b.val,
	};

	return res;
}

#endif

