/*
 *
 * Intel Keystore Linux driver
 * Copyright (c) 2018, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "dal_context.h"
#include "keystore_debug.h"

/* The linked list of DAL contexts. */
struct list_head dal_contexts = LIST_HEAD_INIT(dal_contexts);

/**
 * Initialize one context.
 */
static void init_context_struct(struct dal_keystore_ctx *ctx)
{
	FUNC_BEGIN;

	if (ctx) {
		/* initialize slots of new context */
		INIT_LIST_HEAD(&ctx->slots);
	}

	FUNC_END;
}

/**
 * Deinitialize one context.
 */
static void deinit_context_struct(struct dal_keystore_ctx *ctx)
{
	struct list_head *pos, *q;

	FUNC_BEGIN;

	if (ctx) {
		/* free all slots */
		list_for_each_safe(pos, q, &ctx->slots)	{
			struct dal_keystore_slot *item =
				list_entry(pos, struct dal_keystore_slot,
					   list);
			list_del(pos);
			kzfree(item);
		}
	}

	FUNC_END;
}

/**
 * Free all contexts.
 */
void dal_keystore_free_contexts(void)
{
	struct list_head *pos, *q;

	FUNC_BEGIN;

	list_for_each_safe(pos, q, &dal_contexts) {
		struct dal_keystore_ctx *item =
			list_entry(pos, struct dal_keystore_ctx, list);

		deinit_context_struct(item);
		list_del(pos);
		kzfree(item);
	}

	FUNC_END;
}

/**
 * Allocate a context structure.
 *
 * @return Context structure pointer or NULL if out of memory.
 */
struct dal_keystore_ctx *dal_keystore_allocate_context(void)
{
	struct dal_keystore_ctx *item = NULL;
	struct list_head *pos;
	unsigned int cnt = 0;

	FUNC_BEGIN;

	/* calculate number of clients already registered */
	list_for_each(pos, &dal_contexts) {
		cnt++;
	}

	/* check for maximum number of clients */
	if (cnt < DAL_KEYSTORE_CLIENTS_MAX) {
		/* allocate memory */
		item = kzalloc(sizeof(struct dal_keystore_ctx),
				GFP_KERNEL);
		if (item) {
			init_context_struct(item);

			/* add context to the global list */
			list_add(&(item->list), &dal_contexts);
		}
	}

	FUNC_RES(item);

	return item;
}

/**
 * Free the context, searching by context structure.
 *
 * @param ctx Pointer to the context structure.
 *
 * @return 0 if OK or negative error code (see errno).
 */
int dal_keystore_free_context_ctx(struct dal_keystore_ctx *ctx)
{
	struct list_head *pos, *q;

	FUNC_BEGIN;

	if (!ctx) {
		FUNC_RES(-EFAULT);
		return -EFAULT;
	}

	list_for_each_safe(pos, q, &dal_contexts) {
		struct dal_keystore_ctx *item =
			list_entry(pos, struct dal_keystore_ctx, list);

		if (item == ctx) {
			deinit_context_struct(item);
			list_del(pos);
			kzfree(item);
			FUNC_RES(0);
			return 0;
		}
	}

	FUNC_RES(-EINVAL);

	return -EINVAL;
}

/**
 * Free the context, searching by ClientTicket.
 *
 * @param client_ticket The client ticket (KEYSTORE_CLIENT_TICKET_SIZE bytes).
 *
 * @return 0 if OK or negative error code (see errno).
 */
int dal_keystore_free_context_ticket(const void *client_ticket)
{
	struct list_head *pos, *q;

	FUNC_BEGIN;

	if (!client_ticket) {
		FUNC_RES(-EFAULT);
		return -EFAULT;
	}

	list_for_each_safe(pos, q, &dal_contexts) {
		struct dal_keystore_ctx *item =
			list_entry(pos, struct dal_keystore_ctx, list);

		if (!memcmp(client_ticket, item->client_ticket,
			    KEYSTORE_CLIENT_TICKET_SIZE)) {
			deinit_context_struct(item);
			list_del(pos);
			kzfree(item);
			FUNC_RES(0);
			return 0;
		}
	}

	FUNC_RES(-EINVAL);

	return -EINVAL;
}

/**
 * Find the context, searching by ClientTicket.
 *
 * @param client_ticket The client ticket (KEYSTORE_CLIENT_TICKET_SIZE bytes).
 *
 * @return Context structure pointer or NULL if not found.
 */
struct dal_keystore_ctx *dal_keystore_find_context_ticket(const void *client_ticket)
{
	struct list_head *pos;

	FUNC_BEGIN;

	if (!client_ticket) {
		FUNC_RES(0);
		return NULL;
	}

	list_for_each(pos, &dal_contexts) {
		struct dal_keystore_ctx *item =
			list_entry(pos, struct dal_keystore_ctx, list);

		if (!memcmp(client_ticket, item->client_ticket,
					KEYSTORE_CLIENT_TICKET_SIZE)) {
			FUNC_RES(item);
			return item;
		}
	}

	FUNC_RES(0);

	return NULL;
}

/**
 * Allocate a slot structure in the context.
 *
 * @param ctx Pointer to the context structure.
 *
 * @return Slot structure pointer or NULL if out of memory.
 */
struct dal_keystore_slot *dal_keystore_allocate_slot(struct dal_keystore_ctx *ctx)
{
	struct dal_keystore_slot *item = NULL;
	int id = 0;

	FUNC_BEGIN;

	if (!ctx) {
		FUNC_RES(0);
		return NULL;
	}

	/* find the lowest unused slot ID */
	for (id = 0; id < DAL_KEYSTORE_SLOTS_MAX; id++) {
		if (!dal_keystore_find_slot_id(ctx, id)) {
			/* slot with this ID doesn't exist, we can use it now */
			break;
		}
	}

	if (id < DAL_KEYSTORE_SLOTS_MAX) {
		/* allocate memory */
		item = kzalloc(sizeof(struct dal_keystore_slot),
				GFP_KERNEL);
		if (item) {
			item->slot_id = id;

			/* add slot to the list inside context */
			list_add(&(item->list), &ctx->slots);
		}
	}

	FUNC_RES(item);

	return item;
}

/**
 * Free the slot, searching by slot ID.
 *
 * @param ctx Pointer to the context structure.
 * @param slot_id The slot ID.
 *
 * @return 0 if OK or negative error code (see errno).
 */
int dal_keystore_free_slot_id(struct dal_keystore_ctx *ctx, int slot_id)
{
	struct list_head *pos, *q;

	FUNC_BEGIN;

	if (!ctx) {
		FUNC_RES(-EFAULT);
		return -EFAULT;
	}

	if ((slot_id < 0) || (slot_id >= DAL_KEYSTORE_SLOTS_MAX)) {
		FUNC_RES(-EINVAL);
		return -EINVAL;
	}

	list_for_each_safe(pos, q, &ctx->slots) {
		struct dal_keystore_slot *item =
			list_entry(pos, struct dal_keystore_slot, list);

		if (item) {
			if (item->slot_id == slot_id) {
				list_del(pos);
				kzfree(item);
				FUNC_RES(0);
				return 0;
			}
		}
	}

	FUNC_RES(-EINVAL);

	return -EINVAL;
}

/**
 * Find the slot, searching by slot ID.
 *
 * @param ctx Pointer to the context structure.
 * @param slot_id The slot ID.
 *
 * @return Slot structure pointer or NULL if not found.
 */
struct dal_keystore_slot *dal_keystore_find_slot_id(struct dal_keystore_ctx *ctx,
		int slot_id)
{
	struct list_head *pos;

	FUNC_BEGIN;

	if (!ctx || (slot_id < 0) || (slot_id >= DAL_KEYSTORE_SLOTS_MAX)) {
		FUNC_RES(0);
		return NULL;
	}

	list_for_each(pos, &ctx->slots) {
		struct dal_keystore_slot *item =
			list_entry(pos, struct dal_keystore_slot, list);

		if (item) {
			if (item->slot_id == slot_id) {
				FUNC_RES(item);
				return item;
			}
		}
	}

	FUNC_RES(0);
	return NULL;
}

/**
 * Dump the context contents.
 *
 * @param ctx Pointer to the context structure.
 */
void dal_keystore_dump_ctx(const struct dal_keystore_ctx *ctx)
{
	struct list_head *pos;
	unsigned int cnt = 0;

	if (!ctx)
		return;

	ks_info(KBUILD_MODNAME ": Context at 0x%lx\n",
			(unsigned long) ctx);

	keystore_hexdump("  TICKET", ctx->client_ticket,
			KEYSTORE_CLIENT_TICKET_SIZE);

	list_for_each(pos, &ctx->slots) {
		cnt++;
	}
	ks_info(KBUILD_MODNAME ":   slots: %u\n", cnt);

	if (cnt > 0) {
		list_for_each(pos, &ctx->slots) {
			struct dal_keystore_slot *slot = list_entry(pos,
					struct dal_keystore_slot, list);
			ks_info(KBUILD_MODNAME ":    [%u] appKey size=%u\n",
				slot->slot_id, slot->wrapped_key_size);
			keystore_hexdump("    ", slot->wrapped_key,
					slot->wrapped_key_size);
		}
	}
}
