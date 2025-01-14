/*
 * Copyright (c) 2018-2021 Pavel Shramov <shramov@mexmat.net>
 *
 * tll is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef _TLL_CHANNEL_FILE_H
#define _TLL_CHANNEL_FILE_H

#include "tll/channel/autoseq.h"

#include "tll/util/memoryview.h"

namespace tll::channel {

class File : public tll::channel::AutoSeq<File>
{
	using Base = tll::channel::AutoSeq<File>;

	size_t _block_size = 0;
	size_t _block_init = 0;
	size_t _block_end = 0;
	size_t _offset = 0;

	std::vector<char> _buf;
	std::string _filename;

	enum class Compression : uint8_t { None = 0, LZ4 = 1} _compression;
	bool _autoclose = true;
	bool _end_of_data = false;

	long long _seq_begin;
	long long _seq;

public:
	using frame_size_t = int32_t;
	struct __attribute__((packed)) frame_t
	{
		frame_t(int32_t m, int64_t s) : msgid(m), seq(s) {}
		frame_t(const tll_msg_t *msg) : msgid(msg->msgid), seq(msg->seq) {}
		int32_t msgid;
		int64_t seq;
	};

	static constexpr std::string_view channel_protocol() { return "file"; }
	static constexpr auto process_policy() { return ProcessPolicy::Custom; }

	int _init(const tll::Channel::Url &, tll::Channel *master);
	int _open(const tll::ConstConfig &);
	int _close();

	int _process(long timeout, int flags);
	int _post(const tll_msg_t *msg, int flags);

private:
	int _read_frame(frame_size_t *);
	int _read_data(size_t size, tll_msg_t *msg);

	size_t _data_size(frame_size_t frame) { return frame - sizeof(frame) - 1; }

	int _write_data(const void *data, size_t size) { return _write_datav(tll::const_memory { data, size }); }
	int _write_raw(const void *data, size_t size, size_t _offset);

	template <typename ... Args>
	int _write_datav(Args && ... args);

	int _file_bounds();
	ssize_t _file_size();

	int _seek(long long seq);
	int _block_seq(size_t block, tll_msg_t *msg);
	int _read_seq(frame_size_t frame, tll_msg_t *msg);
	int _read_seq(tll_msg_t *msg);
	int _read_meta();
	int _write_meta();

	int _shift(size_t size);
	int _shift(const tll_msg_t * msg) { return _shift(sizeof(frame_size_t) + sizeof(frame_t) + msg->size + 1); }
	void _truncate(size_t offset);
	int _check_write(size_t size, int r);
};

} // namespace tll::channel

#endif//_TLL_CHANNEL_FILE_H
