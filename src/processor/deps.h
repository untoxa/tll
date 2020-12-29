/*
 * Copyright (c) 2018-2020 Pavel Shramov <shramov@mexmat.net>
 *
 * tll is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef _PROCESSOR_LEVEL_H
#define _PROCESSOR_LEVEL_H

#include "tll/logger.h"
#include "tll/channel.h"
#include "tll/util/url.h"
#include "tll/util/time.h"

#include <list>
#include <string>

namespace tll::processor::_ {

struct Worker;

enum class Shutdown { Close, Error, None };

struct Object
{
	std::unique_ptr<Channel> channel;

	tll_state_t state = TLL_STATE_CLOSED;
	bool decay = false;
	bool opening = false;
	bool verbose = false;

	Shutdown shutdown = Shutdown::None;
	Worker * worker = nullptr;

	std::vector<Object *> depends;
	std::vector<Object *> rdepends;

	std::list<std::string> depends_names; //< Temporary storage used during initialization

	tll::ConstConfig open_parameters;

	tll::time_point open_ts = {};
	tll::duration reopen_delay = {};

	Object(std::unique_ptr<Channel> && ptr)
		: channel(std::move(ptr))
	{
		channel->callback_add(this, TLL_MESSAGE_MASK_STATE);
	}

	int init(const tll::Channel::Url &url);

	int open()
	{
		if (!channel) return EINVAL;
		tll::Props props;
		auto v = open_parameters.get();
		if (v) {
			auto r = Props::parse(*v);
			if (!r)
				return EINVAL;
			props = *r;
		}
		for (auto &[k, c] : open_parameters.browse("**")) {
			auto v = c.get();
			if (v)
				props[k] = *v;
		}
		return channel->open(conv::to_string(props));
	}

	std::string_view name() const { return channel->name(); }

	Channel * get() { return channel.get(); }
	const Channel * get() const { return channel.get(); }

	Channel * operator -> () { return get(); }
	const Channel * operator -> () const { return get(); }

	Channel & operator * () { return *get(); }
	const Channel & operator * () const { return *get(); }

	int callback(const Channel *, const tll_msg_t *);

	bool ready_open()
	{
		if (std::any_of(depends.begin(), depends.end(), [](auto & o) { return o->decay; }))
			return false;
		if (std::any_of(rdepends.begin(), rdepends.end(), [](auto & o) { return o->decay; }))
			return false;
		return std::all_of(depends.begin(), depends.end(), [](auto & o) { return o->state == state::Active; });
	}

	bool ready_close()
	{
		if (std::any_of(rdepends.begin(), rdepends.end(), [](auto & o) { return o->opening; }))
			return false;
		return std::all_of(rdepends.begin(), rdepends.end(), [](auto & o) { return o->state == state::Closed; });
	}
};

} // namespace tll::processor

#endif//_PROCESSOR_LEVEL_H
