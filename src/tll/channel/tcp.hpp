/*
 * Copyright (c) 2018-2021 Pavel Shramov <shramov@mexmat.net>
 *
 * tll is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef _TLL_IMPL_CHANNEL_TCP_HPP
#define _TLL_IMPL_CHANNEL_TCP_HPP

#include "tll/channel/tcp.h"
#include "tll/channel/tcp-scheme.h"

#include "tll/util/conv-fmt.h"
#include "tll/util/size.h"
#include "tll/util/sockaddr.h"

#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __linux__
#include <linux/sockios.h>
#include <linux/net_tstamp.h>
#include <sys/ioctl.h>
#endif

#if defined(__APPLE__) && !defined(MSG_NOSIGNAL)
#  define MSG_NOSIGNAL 0
#endif//__APPLE__

namespace tll::channel {

namespace _ {

inline size_t _fill_iovec(size_t full, struct iovec * iov)
{
	return full;
}

template <typename Arg, typename ... Args>
size_t _fill_iovec(size_t full, struct iovec * iov, const Arg &arg, const Args & ... args)
{
	iov->iov_base = (void *) arg.data;
	iov->iov_len = arg.size;
	return _fill_iovec(full + arg.size, iov + 1, std::forward<const Args &>(args)...);
}

} // namespace _

template <typename T>
int TcpSocket<T>::_init(const tll::Channel::Url &url, tll::Channel *master)
{
	_rbuf.resize(_size);
	_wbuf.resize(_size);
	return 0;
}

template <typename T>
int TcpSocket<T>::_open(const ConstConfig &url)
{
	if (this->fd() == -1) {
		auto fd = url.getT<int>("fd");
		if (!fd)
			return this->_log.fail(EINVAL, "Invalid fd parameter: {}", fd.error());
		this->_update_fd(*fd);
	}
	this->_dcaps_poll(dcaps::CPOLLIN);
	// Fd set by server
	return 0;
}

template <typename T>
int TcpSocket<T>::_close()
{
	auto fd = this->_update_fd(-1);
	if (fd != -1)
		::close(fd);
	return 0;
}

template <typename T>
int TcpSocket<T>::_post_data(const tll_msg_t *msg, int flags)
{
	this->_log.debug("Post {} bytes of data", msg->size);
	int r = send(this->fd(), msg->data, msg->size, MSG_NOSIGNAL | MSG_DONTWAIT);
	if (r < 0)
		return this->_log.fail(errno, "Failed to post data: {}", strerror(errno));
	else if ((size_t) r != msg->size)
		return this->_log.fail(errno, "Failed to post data (truncated): {}", strerror(errno));
	return 0;
}

template <typename T>
int TcpSocket<T>::_post_control(const tll_msg_t *msg, int flags)
{
	if (msg->msgid == tcp_scheme::Disconnect::meta_id()) {
		this->_log.info("Disconnect client on user request");
		this->close();
	}
	return 0;
}

template <typename T>
std::optional<size_t> TcpSocket<T>::_recv(size_t size)
{
	if (_rsize == _rbuf.size()) return EAGAIN;

	auto left = _rbuf.size() - _rsize;
	if (size != 0)
		size = std::min(size, left);
	else
		size = left;
#ifdef __linux__
	struct iovec iov = {_rbuf.data() + _rsize, _rbuf.size() - _rsize};
	msghdr mhdr = {};
	mhdr.msg_iov = &iov;
	mhdr.msg_iovlen = 1;
	mhdr.msg_control = _cbuf.data();
	mhdr.msg_controllen = _cbuf.size();
	int r = recvmsg(this->fd(), &mhdr, MSG_NOSIGNAL | MSG_DONTWAIT);
#else
	int r = recv(this->fd(), _rbuf.data() + _rsize, _rbuf.size() - _rsize, MSG_NOSIGNAL | MSG_DONTWAIT);
#endif
	if (r < 0) {
		if (errno == EAGAIN)
			return 0;
		return this->_log.fail(std::nullopt, "Failed to receive data: {}", strerror(errno));
	} else if (r == 0) {
		this->_log.debug("Connection closed");
		this->channelT()->_on_close();
		return 0;
	}
#ifdef __linux__
	if (mhdr.msg_controllen)
		_timestamp = _cmsg_timestamp(&mhdr);
#endif
	_rsize += r;
	this->_log.trace("Got {} bytes of data", r);
	return r;
}

template <typename T>
int TcpSocket<T>::setup(const tcp_settings_t &settings)
{
	using namespace tll::network;

	_rbuf.resize(settings.buffer_size);

	if (int r = nonblock(this->fd()))
		return this->_log.fail(EINVAL, "Failed to set nonblock: {}", strerror(r));

#ifdef __APPLE__
	if (setsockoptT<int>(this->fd(), SOL_SOCKET, SO_NOSIGPIPE, 1))
		return this->_log.fail(EINVAL, "Failed to set SO_NOSIGPIPE: {}", strerror(errno));
#endif

#ifdef __linux__
	if (settings.timestamping) {
		int v = SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE | SOF_TIMESTAMPING_SOFTWARE;
		if (setsockopt(this->fd(), SOL_SOCKET, SO_TIMESTAMPING, &v, sizeof(v)))
			return this->_log.fail(EINVAL, "Failed to enable timestamping: {}", strerror(errno));
		_cbuf.resize(256);
	}
#endif

	if (settings.keepalive && setsockoptT<int>(this->fd(), SOL_SOCKET, SO_KEEPALIVE, 1))
		return this->_log.fail(EINVAL, "Failed to set keepalive: {}", strerror(errno));

	if (settings.sndbuf && setsockoptT<int>(this->fd(), SOL_SOCKET, SO_SNDBUF, settings.sndbuf))
		return this->_log.fail(EINVAL, "Failed to set sndbuf to {}: {}", settings.sndbuf, strerror(errno));

	if (settings.rcvbuf && setsockoptT<int>(this->fd(), SOL_SOCKET, SO_RCVBUF, settings.rcvbuf))
		return this->_log.fail(EINVAL, "Failed to set rcvbuf to {}: {}", settings.rcvbuf, strerror(errno));

	return 0;
}

template <typename T>
std::chrono::nanoseconds TcpSocket<T>::_cmsg_timestamp(msghdr * msg)
{
	using namespace std::chrono;
	nanoseconds r = {};
#ifdef __linux__
	for (auto cmsg = CMSG_FIRSTHDR(msg); cmsg; cmsg = CMSG_NXTHDR(msg, cmsg)) {
		if(cmsg->cmsg_level != SOL_SOCKET)
			continue;

		if (cmsg->cmsg_type == SO_TIMESTAMPING) {
			auto ts = (struct timespec *) CMSG_DATA(cmsg);
			if (ts[2].tv_sec || ts[2].tv_nsec) // Get HW timestamp if available
				r = seconds(ts[2].tv_sec) + nanoseconds(ts[2].tv_nsec);
			else
				r = seconds(ts->tv_sec) + nanoseconds(ts->tv_nsec);
		}
	}
#endif
	return r;
}

template <typename T>
void TcpSocket<T>::_store_output(const void * base, size_t size, size_t offset)
{
	auto view = tll::make_view(_wbuf, _woff + _wsize);
	auto len = size - offset;
	auto data = offset + (const char *) base;
	if (view.size() < len)
		view.resize(len);
	memcpy(view.data(), data, len);
	_wsize += len;
	if (_wsize == len)
		this->_update_dcaps(dcaps::CPOLLOUT);
}

template <typename T>
int TcpSocket<T>::_sendmsg(const iovec * iov, size_t N)
{
	if (_wsize) {
		auto old = _wsize;
		for (unsigned i = 0; i < N; i++)
			_store_output(iov[i].iov_base, iov[i].iov_len);
		this->_log.debug("Stored {} bytes of pending data (now {})", _wsize - old, _wsize);
		return 0;
	}

	size_t full = 0;
	for (unsigned i = 0; i < N; i++)
		full += iov[i].iov_len;

	struct msghdr msg = {};
	msg.msg_iov = (iovec *) iov;
	msg.msg_iovlen = N;
	auto r = sendmsg(this->fd(), &msg, MSG_NOSIGNAL | MSG_DONTWAIT);
	if (r < 0)
		return this->_log.fail(EINVAL, "Failed to send {} bytes of data: {}", strerror(errno));
	if (r < (ssize_t) full) {
		auto old = _wsize;
		for (unsigned i = 0; i < N; i++) {
			auto len = iov[i].iov_len;
			if (r >= (ssize_t) len) {
				r -= len;
				continue;
			}
			_store_output(iov[i].iov_base, iov[i].iov_len, r);
			r = 0;
		}
		this->_log.debug("Stored {} bytes of pending data (now {})", _wsize - old, _wsize);
	}
	return 0;
}

template <typename T>
template <typename ... Args>
int TcpSocket<T>::_sendv(const Args & ... args)
{
	constexpr unsigned N = sizeof...(Args);
	struct iovec iov[N];
	_::_fill_iovec(0, iov, std::forward<const Args &>(args)...);

	return _sendmsg(iov, N);
}

template <typename T>
int TcpSocket<T>::_process_output()
{
	if (!_wsize)
		return 0;
	auto view = tll::make_view(_wbuf, _woff);
	auto r = ::send(this->fd(), view.data(), _wsize, MSG_NOSIGNAL | MSG_DONTWAIT);
	if (r < 0) {
		if (r == EAGAIN)
			return 0;
		return this->_log.fail(errno, "Failed to send pending data: {}", strerror(errno));
	}
	_woff += r;
	_wsize -= r;
	if (!_wsize) {
		_woff = 0;
		this->_update_dcaps(0, dcaps::CPOLLOUT);
		this->channelT()->_on_output_sent();
	}
	return 0;
}

template <typename T>
int TcpSocket<T>::_process(long timeout, int flags)
{
	auto r = _recv(_rbuf.size());
	if (!r)
		return EINVAL;
	if (!*r)
		return EAGAIN;
	this->_log.debug("Got data: {}", *r);
	tll_msg_t msg = { TLL_MESSAGE_DATA };
	msg.data = _rbuf.data();
	msg.size = *r;
	msg.addr = _msg_addr;
	msg.time = _timestamp.count();
	this->_callback_data(&msg);
	rdone(*r);
	rshift();
	return 0;
}

template <typename T, typename S>
int TcpClient<T, S>::_init(const tll::Channel::Url &url, tll::Channel *master)
{
	this->_msg_addr.fd = 0;

	auto reader = this->channel_props_reader(url);
	auto af = reader.getT("af", network::AddressFamily::UNSPEC);
	this->_size = reader.template getT<util::Size>("size", 128 * 1024);
	_settings.timestamping = reader.getT("timestamping", false);
	_settings.keepalive = reader.getT("keepalive", true);
	_settings.sndbuf = reader.getT("sndbuf", util::Size { 0 });
	_settings.rcvbuf = reader.getT("rcvbuf", util::Size { 0 });
	_settings.buffer_size = reader.getT("buffer-size", util::Size { 64 * 1024 });
	if (!reader)
		return this->_log.fail(EINVAL, "Invalid url: {}", reader.error());

	S::_init(url, master);

	auto host = url.host();
	if (host.size()) {
		auto r = network::parse_hostport(url.host(), af);
		if (!r)
			return this->_log.fail(EINVAL, "Invalid host string '{}': {}", host, r.error());
		_peer = std::move(*r);

		this->_log.debug("Connection to {}:{}", _peer->host, _peer->port);
	} else
		this->_log.debug("Connection address will be provided in open parameters");
	return 0;
}

template <typename T, typename S>
int TcpClient<T, S>::_open(const ConstConfig &url)
{
	tll::network::hostport peer;
	if (!_peer) {
		auto af = url.getT("af", network::AddressFamily::UNSPEC);
		if (!af)
			return this->_log.fail(EINVAL, "Invalid af parameter: {}", af.error());
		auto host = url.get("host");
		if (!host)
			return this->_log.fail(EINVAL, "Remote address not provided in open parameters: no 'host' keyword");
		auto r = network::parse_hostport(*host, *af);
		if (!r)
			return this->_log.fail(EINVAL, "Invalid host string '{}': {}", *host, r.error());
		peer = std::move(*r);
	} else
		peer = *_peer;
	auto addr = tll::network::resolve(peer.af, SOCK_STREAM, peer.host, peer.port);
	if (!addr)
		return this->_log.fail(EINVAL, "Failed to resolve '{}': {}", peer.host, addr.error());
	std::swap(_addr_list, *addr);
	_addr = _addr_list.begin();

	auto fd = socket((*_addr)->sa_family, SOCK_STREAM, 0);
	if (fd == -1)
		return this->_log.fail(errno, "Failed to create socket: {}", strerror(errno));
	this->_update_fd(fd);

	if (this->setup(_settings))
		return this->_log.fail(EINVAL, "Failed to setup socket");

	if (S::_open(url))
		return this->_log.fail(EINVAL, "Parent open failed");

	this->_log.info("Connect to {}", *_addr);
	if (connect(this->fd(), *_addr, _addr->size)) {
		if (errno == EINPROGRESS) {
			this->_dcaps_poll(dcaps::CPOLLOUT);
			return 0;
		}
		return this->_log.fail(errno, "Failed to connect: {}", strerror(errno));
	}

	return this->channelT()->_on_connect();
}

template <typename T, typename S>
int TcpClient<T, S>::_process_connect()
{
	struct pollfd pfd = { this->fd(), POLLOUT };
	auto r = poll(&pfd, 1, 0);
	if (r < 0)
		return this->_log.fail(errno, "Failed to poll: {}", strerror(errno));
	if (r == 0 || (pfd.revents & (POLLOUT | POLLHUP)) == 0)
		return EAGAIN;

	this->_log.info("Connected");

	int err = 0;
	socklen_t len = sizeof(err);
	if (getsockopt(this->fd(), SOL_SOCKET, SO_ERROR, &err, &len))
		return this->_log.fail(errno, "Failed to get connect status: {}", strerror(errno));
	if (err)
		return this->_log.fail(err, "Failed to connect: {}", strerror(err));

	return this->channelT()->_on_connect();
}

template <typename T, typename S>
int TcpClient<T, S>::_process(long timeout, int flags)
{
	if (this->state() == state::Opening)
		return _process_connect();
	return S::_process(timeout, flags);
}

template <typename T>
int TcpServerSocket<T>::_init(const tll::Channel::Url &url, tll::Channel *master)
{
	return 0;
}

template <typename T>
int TcpServerSocket<T>::_open(const ConstConfig &url)
{
	if (this->fd() == -1) {
		auto fd = url.getT<int>("fd");
		if (!fd)
			return this->_log.fail(EINVAL, "Invalid fd parameter: {}", fd.error());
		this->_update_fd(*fd);
	}
	this->_dcaps_poll(dcaps::CPOLLIN);
	return 0;
}

template <typename T>
int TcpServerSocket<T>::_close()
{
	auto fd = this->_update_fd(-1);
	if (fd != -1)
		::close(fd);
	return 0;
}


template <typename T>
int TcpServerSocket<T>::_process(long timeout, int flags)
{
	tll::network::sockaddr_any addr = {};
	addr.size = sizeof(addr.buf);

	tll::network::scoped_socket fd(accept(this->fd(), addr, &addr.size)); //XXX: accept4
	if (fd == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return EAGAIN;
		return this->_log.fail(errno, "Accept failed: {}", strerror(errno));
	}

	if (addr->sa_family != AF_UNIX)
		this->_log.info("Connection {} from {}", fd, addr);
	else
		this->_log.info("Connection {} from {}", fd, "unix socket");

	if (int e = tll::network::nonblock(fd))
		return this->_log.fail(e, "Failed to set nonblock: {}", strerror(e));

#ifdef __APPLE__
	if (tll::network::setsockoptT<int>(fd, SOL_SOCKET, SO_NOSIGPIPE, 1))
		return this->_log.fail(EINVAL, "Failed to set SO_NOSIGPIPE: {}", strerror(errno));
#endif

	tll_msg_t msg = {};
	tcp_connect_t data = {};
	data.fd = fd;
	data.addrlen = addr.size;
	data.addr = addr;

	msg.type = TLL_MESSAGE_DATA;
	msg.size = sizeof(data);
	msg.data = &data;
	this->_callback_data(&msg);
	fd.release();
	return 0;
}

template <typename T, typename C>
int TcpServer<T, C>::_init(const tll::Channel::Url &url, tll::Channel *master)
{
	auto reader = this->channelT()->channel_props_reader(url);
	auto af = reader.getT("af", network::AddressFamily::UNSPEC);
	_settings.timestamping = reader.getT("timestamping", false);
	_settings.keepalive = reader.getT("keepalive", true);
	_settings.sndbuf = reader.getT("sndbuf", util::Size { 0 });
	_settings.rcvbuf = reader.getT("rcvbuf", util::Size { 0 });
	_settings.buffer_size = reader.getT("buffer-size", util::Size { 64 * 1024 });
	if (!reader)
		return this->_log.fail(EINVAL, "Invalid url: {}", reader.error());

	auto host = url.host();
	auto r = network::parse_hostport(url.host(), af);
	if (!r)
		return this->_log.fail(EINVAL, "Invalid host string '{}': {}", host, r.error());
	_af = r->af;
	_host = r->host;
	_port = r->port;

	this->_scheme_control.reset(this->context().scheme_load(tcp_scheme::scheme_string));
	if (!this->_scheme_control.get())
		return this->_log.fail(EINVAL, "Failed to load control scheme");

	this->_log.debug("Listen on {}:{}", _host, _port);
	return 0;
}

template <typename T, typename C>
int TcpServer<T, C>::_open(const ConstConfig &url)
{
	_cleanup_flag = false;
	_addr_seq = 0;

	auto addr = tll::network::resolve(_af, SOCK_STREAM, _host.c_str(), _port);
	if (!addr)
		return this->_log.fail(EINVAL, "Failed to resolve '{}': {}", _host, addr.error());

	for (auto & a : *addr) {
		if (this->_bind(a))
			return this->_log.fail(EINVAL, "Failed to listen on {}", a);
	}

	this->state(state::Active);
	return 0;
}

template <typename T, typename C>
int TcpServer<T, C>::_bind(const tll::network::sockaddr_any &addr)
{
	this->_log.info("Listen on {}", conv::to_string(addr));

#ifdef SOCK_NONBLOCK
	static constexpr int sflags = SOCK_STREAM | SOCK_NONBLOCK;
#else
	static constexpr int sflags = SOCK_STREAM;
#endif

	tll::network::scoped_socket fd(socket(addr->sa_family, sflags, 0));
	if (fd == -1)
		return this->_log.fail(errno, "Failed to create socket: {}", strerror(errno));

#ifndef SOCK_NONBLOCK
	if (int r = nonblock(fd))
		return this->_log.fail(EINVAL, "Failed to set nonblock: {}", strerror(r));
#endif

	int flag = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)))
		return this->_log.fail(EINVAL, "Failed to set SO_REUSEADDR: {}", strerror(errno));

	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag)))
		return this->_log.fail(EINVAL, "Failed to set SO_KEEPALIVE: {}", strerror(errno));

	if (bind(fd, addr, addr.size))
		return this->_log.fail(errno, "Failed to bind: {}", strerror(errno));

	if (listen(fd, 10))
		return this->_log.fail(errno, "Failed to listen on socket: {}", strerror(errno));

	auto r = this->context().channel(fmt::format("tcp://;fd-mode=yes;tll.internal=yes;name={}/{}", this->name, fd), this->self(), &tcp_server_socket_t::impl);
	if (!r)
		return this->_log.fail(EINVAL, "Failed to init server socket channel");

	auto c = channel_cast<tcp_server_socket_t>(r.get());
	c->bind(fd.release());
	tll_channel_callback_add(r.get(), _cb_socket, this, TLL_MESSAGE_MASK_ALL);

	this->_child_add(r.get());
	_sockets.emplace_back((tll::Channel *) r.release());

	if (c->open(tll::ConstConfig()))
		return this->_log.fail(EINVAL, "Failed to open server socket channel");

	return 0;
}

template <typename T, typename C>
int TcpServer<T, C>::_close()
{
	if (this->_af == AF_UNIX && _sockets.size()) {
		this->_log.info("Unlink unix socket {}", this->_host);
		if (unlink(this->_host.c_str()))
			this->_log.warning("Failed to unlink socket {}: {}", this->_host, strerror(errno));
	}
	for (auto & c : _clients)
		tll_channel_free(*c.second);
	_clients.clear();
	_sockets.clear();
	return 0;
}

template <typename T, typename C>
typename TcpServer<T, C>::tcp_socket_t * TcpServer<T, C>::_lookup(const tll_addr_t &a)
{
	auto addr = tcp_socket_addr_t::cast(&a);
	if (addr->fd == -1)
		return this->_log.fail(nullptr, "Invalid address");
	auto i = _clients.find(addr->fd);
	if (i == _clients.end())
		return this->_log.fail(nullptr, "Address not found: {}/{}", addr->fd, addr->seq);
	if (addr->seq != i->second->msg_addr().seq)
		return this->_log.fail(nullptr, "Address seq mismatch: {} != {}", addr->seq, i->second->msg_addr().seq);
	return i->second;
}

template <typename T, typename C>
int TcpServer<T, C>::_post(const tll_msg_t *msg, int flags)
{
	auto socket = _lookup(msg->addr);
	if (!socket)
		return EINVAL;
	return socket->post(msg, flags);
}

template <typename T, typename C>
void TcpServer<T, C>::_cleanup()
{
	if (!_cleanup_flag) return;

	for (auto i = _clients.begin(); i != _clients.end();)
	{
		switch (i->second->state()) {
		case state::Error:
		case state::Closed:
			_cleanup(i->second);
			i = _clients.erase(i);
			break;
		default:
			i++;
			break;
		}
	}

	_cleanup_flag = false;
}

template <typename T, typename C>
void TcpServer<T, C>::_cleanup(tcp_socket_t * c)
{
	this->_log.debug("Cleanup client {} @{}", c->name, (void *) c);
	this->_child_del(*c);
	delete c;
}

template <typename T, typename C>
int TcpServer<T, C>::_cb_other(const tll_channel_t *c, const tll_msg_t *msg)
{
	auto socket = tll::channel_cast<tcp_socket_t>(const_cast<tll_channel_t *>(c))->channelT();
	if (msg->type == TLL_MESSAGE_STATE) {
		if (msg->msgid == state::Error) {
			this->channelT()->_on_child_error(socket);
			_cleanup_flag = true;
		} else if (msg->msgid == state::Closing) {
			this->channelT()->_on_child_closing(socket);
			_cleanup_flag = true;
		}
	} else if (msg->type == TLL_MESSAGE_CONTROL)
		this->_callback(msg);
	return 0;
}

template <typename T, typename C>
void TcpServer<T, C>::_on_child_connect(tcp_socket_t *socket, const tcp_connect_t * conn)
{
	std::array<char, tcp_scheme::Connect::meta_size()> buf;
	auto connect = tcp_scheme::Connect::bind(buf);
	if (conn->addr->sa_family == AF_INET) {
		auto in = (const sockaddr_in *) conn->addr;
		connect.get_host().set_ipv4(in->sin_addr.s_addr);
		connect.set_port(ntohs(in->sin_port));
	} else if (conn->addr->sa_family == AF_INET6) {
		auto in6 = (const sockaddr_in6 *) conn->addr;
		connect.get_host().set_ipv6({(const char *) &in6->sin6_addr, 16 });
		connect.set_port(ntohs(in6->sin6_port));
	} else if (conn->addr->sa_family == AF_UNIX) {
		connect.get_host().set_unix(0);
	}
	tll_msg_t msg = { TLL_MESSAGE_CONTROL };
	msg.msgid = connect.meta_id();
	msg.size = connect.view().size();
	msg.data = connect.view().data();
	msg.addr = socket->msg_addr();
	this->_callback(&msg);
}

template <typename T, typename C>
void TcpServer<T, C>::_on_child_closing(tcp_socket_t *socket)
{
	tll_msg_t m = { TLL_MESSAGE_CONTROL };
	m.msgid = tcp_scheme::Disconnect::meta_id();
	m.addr = socket->msg_addr();
	this->_callback(&m);
}

template <typename T, typename C>
int TcpServer<T, C>::_cb_data(const tll_channel_t *c, const tll_msg_t *msg)
{
	return this->_callback_data(msg);
}

template <typename T, typename C>
int TcpServer<T, C>::_cb_socket(const tll_channel_t *c, const tll_msg_t *msg)
{
	_cleanup();

	if (msg->type != TLL_MESSAGE_DATA) {
		if (msg->type == TLL_MESSAGE_STATE) {
			switch (msg->msgid) {
			case state::Error:
				this->_log.error("Listening socket channel failed");
				this->state(tll::state::Error);
				break;
			default:
				break;
			}
		}
		return 0;
	}
	if (msg->size < sizeof(tcp_connect_t))
		return this->_log.fail(EMSGSIZE, "Invalid connect data size: {} < {}", msg->size, sizeof(tcp_connect_t));
	auto conn = (const tcp_connect_t *) msg->data;
	auto fd = conn->fd;
	this->_log.debug("Got connection fd {}", fd);
	if (this->state() != tll::state::Active) {
		this->_log.debug("Close incoming connection, current state is {}", tll_state_str(this->state()));
		::close(fd);
		return 0;
	}

	auto r = this->context().channel(fmt::format("tcp://;fd-mode=yes;tll.internal=yes;name={}/{}", this->name, fd), this->self(), &tcp_socket_t::impl);
	if (!r)
		return this->_log.fail(EINVAL, "Failed to init client socket channel");

	auto client = channel_cast<tcp_socket_t>(r.get());
	//r.release();
	client->bind(fd);
	client->setup(_settings);
	tll_channel_callback_add(r.get(), _cb_other, this, TLL_MESSAGE_MASK_STATE | TLL_MESSAGE_MASK_CONTROL);
	tll_channel_callback_add(r.get(), _cb_data, this, TLL_MESSAGE_MASK_DATA);
	if (this->channelT()->_on_accept(r.get())) {
		this->_log.debug("Client channel rejected");
		return 0;
	}

	auto it = _clients.find(fd);
	if (it != _clients.end()) {
		_cleanup(it->second);
		it->second = client;
	} else
		_clients.emplace(fd, client);
	this->_child_add(r.release());
	client->open(tll::ConstConfig());

	this->channelT()->_on_child_connect(client, conn);

	return 0;
}

} // namespace tll::channel

#endif//_TLL_IMPL_CHANNEL_TCP_HPP
