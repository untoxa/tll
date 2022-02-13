/*
 * Copyright (c) 2018-2021 Pavel Shramov <shramov@mexmat.net>
 *
 * tll is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef _TLL_CHANNEL_GENPREFIX_H
#define _TLL_CHANNEL_GENPREFIX_H

#include "tll/channel/prefix.h"

namespace tll::channel {
class GenPrefix : public Prefix<GenPrefix>
{
	using Base = Prefix<GenPrefix>;

 	size_t _count = 100000;
	long long _seq = -1;
	long long _end = -1;
	bool _snapshot = false;

	std::unique_ptr<Channel> _first;
 public:
	static constexpr std::string_view channel_protocol() { return "gen+"; }

	int _on_init(tll::Channel::Url &curl, const tll::Channel::Url &url, const tll::Channel * master)
	{
		_first = this->context().channel(fmt::format("timer://;interval=1ms;name={}/snapshot;tll.internal=yes;dump=frame", name));
		if (!_first)
			return _log.fail(EINVAL, "Failed to create snapshot channel");
		_first->callback_add([](const tll_channel_t *, const tll_msg_t *m, void *user) -> int { static_cast<GenPrefix *>(user)->_on_snapshot(m); return 0; }, this, TLL_MESSAGE_MASK_DATA);
		return 0;
	}

	int _open(const tll::ConstConfig &params)
	{
		_seq = _end = -1;
		_snapshot = true;
		_child_add(_first.get(), "snapshot");
		if (_first->open())
			return _log.fail(EINVAL, "Failed to open snapshot channel");
		return Base::_open(params);
	}

	int _on_data(const tll_msg_t *msg)
	{
		if (_snapshot)
			return 0;
		_end += _count;
		_update_dcaps(dcaps::Process | dcaps::Pending);
		return 0;
	}

	int _process(long timeout, int flags)
	{
		if (_snapshot)
			return EAGAIN;
		if (_end == _seq) {
			_update_dcaps(0, dcaps::Process | dcaps::Pending);
			return EAGAIN;
		}
		tll_msg_t msg = { TLL_MESSAGE_DATA };
		msg.seq = ++_seq;
		_callback_data(&msg);
		return 0;
	}

 private:
	void _on_snapshot(const tll_msg_t *msg)
	{
		_log.info("Snapshot end: {}, limit {}", _end, 100 * _count);
		_end += _count;
		if (_end < 100 * (long long) _count)
			return;
		_first->close();
		_snapshot = false;
		_child_del(_first.get(), "snapshot");
		_update_dcaps(dcaps::Process | dcaps::Pending);
	}
};

} // namespace tll::channel

#endif//_TLL_CHANNEL_GENPREFIX_H
