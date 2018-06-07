/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_ENERGY_MODEL_H
#define _LINUX_ENERGY_MODEL_H
#include <linux/types.h>
#include <linux/cpumask.h>
#include <linux/jump_label.h>
#include <linux/rcupdate.h>
#include <linux/kobject.h>
#include <linux/sched/cpufreq.h>

#ifdef CONFIG_ENERGY_MODEL
struct em_cap_state {
	unsigned long capacity;
	unsigned long frequency;
	unsigned long power;
};

struct em_cs_table {
	struct em_cap_state *state;
	int nr_cap_states;
	struct rcu_head rcu;
};

struct em_freq_domain {
	struct em_cs_table *cs_table;
	cpumask_t cpus;
	struct kobject kobj;
};

struct em_data_callback {
	/**
	 * active_power() - Provide power at the next capacity state of a CPU
	 * @power	: Active power at the capacity state (modified)
	 * @freq	: Frequency at the capacity state (modified)
	 * @cpu		: CPU for which we do this operation
	 *
	 * active_power() must find the lowest capacity state of 'cpu' above
	 * 'freq' and update 'power' and 'freq' to the matching active power
	 * and frequency.
	 *
	 * Return 0 on success.
	 */
	int (*active_power) (unsigned long *power, unsigned long *freq, int cpu);
};

int em_register_freq_domain(cpumask_t *span, unsigned int nr_states,
						struct em_data_callback *cb);
void em_rescale_cpu_capacity(void);
struct em_freq_domain *em_cpu_get(int cpu);

/**
 * em_fd_energy() - Estimates the energy consumed by the CPUs of a freq. domain
 * @fd		: frequency domain for which energy has to be estimated
 * @max_util	: highest utilization among CPUs of the domain
 * @sum_util	: sum of the utilization of all CPUs in the domain
 *
 * Return: the sum of the energy consumed by the CPUs of the domain assuming
 * a capacity state satisfying the max utilization of the domain.
 */
static inline unsigned long em_fd_energy(struct em_freq_domain *fd,
				unsigned long max_util, unsigned long sum_util)
{
	struct em_cs_table *cs_table;
	struct em_cap_state *cs;
	unsigned long freq;
	int i;

	cs_table = rcu_dereference(fd->cs_table);
	if (!cs_table)
		return 0;

	/* Map the utilization value to a frequency */
	cs = &cs_table->state[cs_table->nr_cap_states - 1];
	freq = map_util_freq(max_util, cs->frequency, cs->capacity);

	/* Find the lowest capacity state above this frequency */
	for (i = 0; i < cs_table->nr_cap_states; i++) {
		cs = &cs_table->state[i];
		if (cs->frequency >= freq)
			break;
	}

	return cs->power * sum_util / cs->capacity;
}

/**
 * em_fd_nr_cap_states() - Get the number of capacity states of a freq. domain
 * @fd		: frequency domain for which want to do this
 *
 * Return: the number of capacity state in the frequency domain table
 */
static inline int em_fd_nr_cap_states(struct em_freq_domain *fd)
{
	struct em_cs_table *table = rcu_dereference(fd->cs_table);

	return table->nr_cap_states;
}

#else
struct em_freq_domain;
struct em_data_callback;
static inline int em_register_freq_domain(cpumask_t *span,
			unsigned int nr_states, struct em_data_callback *cb)
{
	return -ENOTSUPP;
}
static inline struct em_freq_domain *em_cpu_get(int cpu)
{
	return NULL;
}
static inline unsigned long em_fd_energy(struct em_freq_domain *fd,
			unsigned long max_util, unsigned long sum_util)
{
	return 0;
}
static inline int em_fd_nr_cap_states(struct em_freq_domain *fd)
{
	return 0;
}
static inline void em_rescale_cpu_capacity(void) { }
#endif

#endif
