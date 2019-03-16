/*

  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2014 - 2018 Intel Corporation.

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  Contact Information:
  SoC Watch Developer Team <socwatchdevelopers@intel.com>
  Intel Corporation,
  1300 S Mopac Expwy,
  Austin, TX 78746

  BSD LICENSE

  Copyright(c) 2014 - 2018 Intel Corporation.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef _SWHV_STRUCTS_H_
#define _SWHV_STRUCTS_H_ 1

#include "sw_structs.h"

enum swhv_hypervisor_type {
	swhv_hypervisor_none = 0,
	swhv_hypervisor_mobilevisor,
	swhv_hypervisor_acrn,
};

/*
 * Structure to return version information.
 */
#pragma pack(push)
#pragma pack(1)
struct sp_driver_version_info {
	pw_s32_t major;
	pw_s32_t minor;
	pw_s32_t other;
};

struct spdrv_ioctl_arg {
	pw_s32_t in_len;
	pw_s32_t out_len;
	char *in_arg;
	char *out_arg;
};
#pragma pack(pop)

/*
 * Various commands to control a collection.
 */
enum swhvdrv_cmd {
	SWHVDRV_CMD_START,
	SWHVDRV_CMD_STOP,
	/* others here when appropriate */
	SVHVDRV_CMD_MAX
};

enum swhv_collector_type {
	SWHV_COLLECTOR_TYPE_NONE,
	SWHV_COLLECTOR_TYPE_SWITCH,
	SWHV_COLLECTOR_TYPE_MSR,
};

enum swhv_io_cmd { SWHV_IO_CMD_READ = 0, SWHV_IO_CMD_WRITE, SWHV_IO_CMD_MAX };

#pragma pack(push, 1)
struct swhv_driver_msr_io_descriptor {
	pw_u64_t address;
	enum sw_msr_type type;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct swhv_driver_switch_io_descriptor {
	pw_u32_t switch_bitmask;
};
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct swhv_driver_io_descriptor {
	pw_u16_t collection_type; // One of 'enum swhv_collector_type'
	pw_s16_t collection_command; // One of 'enum swhv_io_cmd'
	pw_u16_t counter_size_in_bytes; // The number of bytes to READ or WRITE
	union {
		struct swhv_driver_msr_io_descriptor msr_descriptor;
		struct swhv_driver_switch_io_descriptor switch_descriptor;
	};
	pw_u64_t write_value; // The value to WRITE
} swhv_driver_io_descriptor_t;
#pragma pack(pop)

#pragma pack(push, 1)
struct swhv_driver_interface_info {
	pw_s16_t cpu_mask; // On which CPU(s) should the driver read the data?
		// Currently:  -2 ==> read on ALL CPUs,
		//             -1 ==> read on ANY CPU,
		//           >= 0 ==> the specific CPU to read on
	pw_s16_t sample_id; // Sample ID, used to map it back to Metric Plugin, Metric and Msg ID combo
	pw_u16_t num_io_descriptors; // Number of descriptors in the array, below.
	pw_u8_t descriptors[1]; // Array of swhv_driver_io_descriptor structs.
};
#pragma pack(pop)
#define SWHV_DRIVER_INTERFACE_INFO_HEADER_SIZE()                               \
	(sizeof(struct swhv_driver_interface_info) - sizeof(pw_u8_t[1]))

#pragma pack(push, 1)
struct swhv_driver_interface_msg {
	pw_u16_t num_infos; // Number of 'swhv_driver_interface_info' structs contained within the 'infos' variable, below
	// pw_u16_t infos_size_bytes; // Size of data inlined within the 'infos' variable, below
	pw_u8_t infos[1];
};
#pragma pack(pop)
#define SWHV_DRIVER_INTERFACE_MSG_HEADER_SIZE()                                \
	(sizeof(struct swhv_driver_interface_msg) - sizeof(pw_u8_t[1]))

/*
 * ACRN specific structs, copied from the ACRN profiling service
 * DO NOT modify these below stucts
 */
#define SBUF_HEAD_SIZE 64 /* bytes */

typedef enum PROFILING_SOCWATCH_FEATURE {
	SOCWATCH_COMMAND = 0,
	SOCWATCH_VM_SWITCH_TRACING,
	MAX_SOCWATCH_FEATURE_ID,
} profiling_socwatch_feature;

typedef enum PROFILING_SOCWATCH_FEATURE acrn_type;

/*
 * current default ACRN header
 */
struct data_header {
	uint32_t collector_id;
	uint16_t cpu_id;
	uint16_t data_type;
	uint64_t tsc;
	uint64_t payload_size;
	uint64_t reserved;
} __attribute__((aligned(32)));
#define ACRN_MSG_HEADER_SIZE (sizeof(struct data_header))

struct vm_switch_trace {
	int32_t os_id;
	uint64_t vmenter_tsc;
	uint64_t vmexit_tsc;
	uint64_t vmexit_reason;
} __attribute__((aligned(32)));
#define VM_SWITCH_TRACE_SIZE (sizeof(struct vm_switch_trace))

#define MAX_NR_VCPUS 8
#define MAX_NR_VMS 6

struct profiling_vcpu_pcpu_map {
	int32_t vcpu_id;
	int32_t pcpu_id;
	int32_t apic_id;
} __attribute__((aligned(8)));

struct profiling_vm_info {
	int32_t vm_id;
	unsigned char guid[16];
	char vm_name[16];
	int32_t num_vcpus;
	struct profiling_vcpu_pcpu_map cpu_map[MAX_NR_VCPUS];
} __attribute__((aligned(8)));

struct profiling_vm_info_list {
	int32_t num_vms;
	struct profiling_vm_info vm_list[MAX_NR_VMS];
} __attribute__((aligned(8)));

/*
 * End of ACRN specific structs, copied from the ACRN profiling service
 */
typedef struct data_header acrn_msg_header;
typedef struct vm_switch_trace vmswitch_trace_t;

/*
 * ACRN specific constants shared between the driver and user-mode
 */
// Per CPU buffer size
#define ACRN_BUF_SIZE ((4 * 1024 * 1024) - SBUF_HEAD_SIZE /* 64 bytes */)
// Size of buffer at which data should be transferred to user-mode
#define ACRN_BUF_TRANSFER_SIZE (ACRN_BUF_SIZE / 2)
/*
 * The ACRN 'sbuf' buffers consist of fixed size elements.
 * This is how they are intended to be used, though SoCWatch only uses it to
 * allocate the correct buffer size.
 */
#define ACRN_BUF_ELEMENT_SIZE 32 /* byte */
#define ACRN_BUF_ELEMENT_NUM (ACRN_BUF_SIZE / ACRN_BUF_ELEMENT_SIZE)
#define ACRN_BUF_FILLED_SIZE(sbuf) (sbuf->size - sbuf_available_space(sbuf))

#endif // _SWHV_STRUCTS_H_
