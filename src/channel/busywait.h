/*
 * Copyright (c) 2018-2021 Pavel Shramov <shramov@mexmat.net>
 *
 * tll is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef _TLL_CHANNEL_BUSYWAIT_H
#define _TLL_CHANNEL_BUSYWAIT_H

#include "tll/channel/prefix.h"
#include "tll/util/time.h"

#include <thread>

namespace tll::channel {
class BusyWait : public Prefix<BusyWait>
{
	using Base = Prefix<BusyWait>;

	tll::duration _timeout;

 public:
	static constexpr std::string_view channel_protocol() { return "busywait+"; }

	int _init(const tll::Channel::Url &url, Channel *parent)
	{
		using namespace std::chrono_literals;

		auto reader = channel_props_reader(url);
		_timeout = reader.getT<tll::duration>("delay", 1ms);
		if (!reader)
			return _log.fail(EINVAL, "Invalid url: {}", reader.error());
		return Base::_init(url, parent);
	}

	int _on_data(const tll_msg_t *msg)
	{
		using namespace std::chrono_literals;

		if (_timeout < 1ms) {
			auto end = tll::time::now() + _timeout;
			while (tll::time::now() < end) {
			}
		} else
			std::this_thread::sleep_for(_timeout);
		_callback_data(msg);
		return 0;
	}
};

} // namespace tll::channel

#endif//_TLL_CHANNEL_BUSYWAIT_H
