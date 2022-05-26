// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022 NXP
 */
#ifndef S32_FP_H
#define S32_FP_H

#define FP_PRECISION		100000000U
/* 1 percent error */
#define FP_PRECISION_ERROR	(FP_PRECISION / 100U)

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
	return (val.val + FP_PRECISION_ERROR) / FP_PRECISION;
}

static inline void __reduce_factors(uint64_t *a, uint64_t *b)
{
	uint64_t factor;

	for (factor = 2; factor <= 5; factor++) {
		while (*a % factor == 0 && *b % factor == 0) {
			*a /= factor;
			*b /= factor;
		}
	}
}

static inline void __reduce_precision(uint64_t a, uint64_t b,
				      uint64_t *precision_lost)
{
	uint64_t result;
	bool overflow;

	*precision_lost = 1;

	if (!a || !b)
		return;

	do {
		overflow = false;
		result = a * b;

		if (result / a != b) {
			*precision_lost <<= 1;
			a >>= 1;
			b >>= 1;
			overflow = true;
		}
	} while (overflow);
}

static inline struct fp_data fp_div(struct fp_data a, struct fp_data b)
{
	struct fp_data res;
	uint64_t div_factor = FP_PRECISION;
	uint64_t prec_factor;

	/* Avoid overflow if possible */
	__reduce_factors(&a.val, &b.val);
	__reduce_factors(&div_factor, &b.val);

	__reduce_precision(a.val, div_factor, &prec_factor);

	/* Equivalent of a.val * div_factor / b.val */
	a.val /= prec_factor;
	b.val /= prec_factor;
	div_factor /= prec_factor;

	res.val = a.val * div_factor / b.val;

	res.val *= prec_factor;
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

