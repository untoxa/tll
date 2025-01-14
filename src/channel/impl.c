/*
 * Copyright (c) 2018-2021 Pavel Shramov <shramov@mexmat.net>
 *
 * tll is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "tll/channel/impl.h"

#include <errno.h>
#include <string.h>

void tll_channel_list_free(tll_channel_list_t *l)
{
	if (!l) return;
	tll_channel_list_free(l->next);
	free(l);
}

int tll_channel_list_add(tll_channel_list_t **l, tll_channel_t *c)
{
	tll_channel_list_t ** i = l;
	if (!l) return EINVAL;
	for (; *i; i = &(*i)->next) {
		if ((*i)->channel == c)
			return EEXIST;
	}
	*i = (tll_channel_list_t *) malloc(sizeof(tll_channel_list_t));
	memset(*i, 0, sizeof(tll_channel_list_t));
	(*i)->channel = c;
	return 0;
}

int tll_channel_list_del(tll_channel_list_t **l, const tll_channel_t *c)
{
	tll_channel_list_t ** i = l;
	if (!l) return EINVAL;
	for (; *i; i = &(*i)->next) {
		if ((*i)->channel == c) {
			tll_channel_list_t * tmp = *i;
			*i = (*i)->next;
			free(tmp);
			return 0;
		}
	}
	return ENOENT;
}

void tll_channel_internal_init(tll_channel_internal_t *ptr)
{
	memset(ptr, 0, sizeof(tll_channel_internal_t));
	ptr->fd = -1;
}

void tll_channel_internal_clear(tll_channel_internal_t *ptr)
{
	tll_channel_list_free(ptr->children);
	ptr->children = NULL;

	free(ptr->cb);
	ptr->cb = NULL;
	ptr->cb_size = 0;

	free(ptr->data_cb);
	ptr->data_cb = NULL;
	ptr->data_cb_size = 0;
}

static int _state_callback(const tll_channel_t * c, const tll_msg_t *msg, void * data)
{
	tll_channel_internal_t * ptr = (tll_channel_internal_t *) data;
	if (!ptr)
		return EINVAL;
	if (msg->type != TLL_MESSAGE_STATE || msg->msgid != TLL_STATE_DESTROY)
		return 0;
	return tll_channel_internal_child_del(ptr, c, NULL, 0);
}

int tll_channel_internal_child_add(tll_channel_internal_t *ptr, tll_channel_t *c, const char * tag, int len)
{
	//tll_logger_printf(ptr->logger, TLL_LOGGER_INFO, "Add child %s", tll_channel_name(c));
	int r = tll_channel_list_add(&ptr->children, c);
	if (r) {
		//tll_logger_printf(internal->logger, TLL_LOGGER_ERROR, "Failed to add child '%s': %s", tll_channel_name(c), strerror(r));
		return r;
	}
	tll_msg_t msg = {TLL_MESSAGE_CHANNEL, TLL_MESSAGE_CHANNEL_ADD};
	msg.data = &c;
	msg.size = sizeof(c);
	tll_channel_callback_add(c, _state_callback, ptr, TLL_MESSAGE_MASK_STATE);
	tll_channel_callback(ptr, &msg);
	if (tag && (len > 0 || strlen(tag) > 0))
		tll_config_set_config(ptr->config, tag, len, tll_channel_config(c), 1);
	return 0;
}

int tll_channel_internal_child_del(tll_channel_internal_t *ptr, const tll_channel_t *c, const char * tag, int len)
{
	//tll_logger_printf(log, TLL_LOGGER_INFO, "Remove child %s", tll_channel_name(c));
	int r = tll_channel_list_del(&ptr->children, c);
	if (r) {
		//tll_logger_printf(internal->logger, TLL_LOGGER_ERROR, "Failed to remove child '%s': %s", tll_channel_name(c), strerror(r));
		return r;
	}
	tll_msg_t msg = {TLL_MESSAGE_CHANNEL, TLL_MESSAGE_CHANNEL_DELETE};
	msg.data = &c;
	msg.size = sizeof(c);
	tll_channel_callback(ptr, &msg);
	if (tag && (len > 0 || strlen(tag) > 0))
		tll_config_remove(ptr->config, tag, len);
	tll_channel_callback_del((tll_channel_t *) c, _state_callback, ptr, TLL_MESSAGE_MASK_STATE);
	return 0;
}
