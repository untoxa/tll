#pragma once

#include <tll/scheme/binder.h>
#include <tll/util/conv.h>

namespace control_scheme {

static constexpr std::string_view scheme_string = R"(yamls+gz://eJydVF1v2jAUfe+v8JtfQCKMpsDbBO02qVsrdes0VX1wk5vUarAz22FjFf+9147zBQSmvoDje3TuzTnnZkgEW8GcUHpGCIhiped4IITeg9JcCjonr2aTI6LgwgThwIHwki4KpUAYBATb7dmw4llIkfD0ExhLyGOsjvCQcMhiTz0krx6bM/NMB6Tkp9ooLlK6x3XPsgIqtnE/2wtsDpB1MWvHdbLlpYirhh9GTfHmpbqdtG4vlZKqKpz3zwcOd6T3HZhrmV7DGrKKLhy1belRUUHC/5589czx1ihr5xQfZW7QZm0dtRVqB8V+dNtyWnHDI5Zh7XxA6BKeitTaPqjeHfXA8xeRSDyO8fhdschSjfD8kylh50Ept7szKYgKjNka3jXXN+lb/AK9G8Nb29KLeNEy61Y299PW/WfIMlkVZv0mrv1adOYNwj21Nag1j45G7c4wA8tilTehmoU75R95jL81IAjGO2vqUN0lnTYKfYyMFbc0ZZFJDbGXzD54V5ylOJ7ceIPbpt7k4N0LnHtP3FStrzKW6v3vg0OQh1oIw1Irg0wSDcY11/wfWL7Hba/M0TMToh3XnlBr9/o1qlRjF5S4QWtQOfchI1prPwnG0wbxFbS2r3F6GV1W61ZlNv5vyaRACzK/V0tmmHVqb2Ps32lV4HdnhnCyB2FxrLohPgCK7RRHEuxluZLqD1Mt6S7CXn1iTNrJ+bt9K/HbX+kyHi7RTdvZ+B15egMKzgxo)";

enum class Version: uint16_t
{
	Current = 1,
};

struct ConfigGet
{
	static constexpr size_t meta_size() { return 8; }
	static constexpr std::string_view meta_name() { return "ConfigGet"; }
	static constexpr int meta_id() { return 10; }
	static constexpr size_t offset_path = 0;

	template <typename Buf>
	struct binder_type : public tll::scheme::Binder<Buf>
	{
		using tll::scheme::Binder<Buf>::Binder;

		static constexpr auto meta_size() { return ConfigGet::meta_size(); }
		static constexpr auto meta_name() { return ConfigGet::meta_name(); }
		static constexpr auto meta_id() { return ConfigGet::meta_id(); }
		void view_resize() { this->_view_resize(meta_size()); }

		std::string_view get_path() const { return this->template _get_string<tll_scheme_offset_ptr_t>(offset_path); }
		void set_path(std::string_view v) { return this->template _set_string<tll_scheme_offset_ptr_t>(offset_path, v); }
	};

	template <typename Buf>
	static binder_type<Buf> bind(Buf &buf, size_t offset = 0) { return binder_type<Buf>(tll::make_view(buf).view(offset)); }

	template <typename Buf>
	static binder_type<Buf> bind_reset(Buf &buf) { return tll::scheme::make_binder_reset<binder_type, Buf>(buf); }
};

struct ConfigValue
{
	static constexpr size_t meta_size() { return 16; }
	static constexpr std::string_view meta_name() { return "ConfigValue"; }
	static constexpr int meta_id() { return 20; }
	static constexpr size_t offset_key = 0;
	static constexpr size_t offset_value = 8;

	template <typename Buf>
	struct binder_type : public tll::scheme::Binder<Buf>
	{
		using tll::scheme::Binder<Buf>::Binder;

		static constexpr auto meta_size() { return ConfigValue::meta_size(); }
		static constexpr auto meta_name() { return ConfigValue::meta_name(); }
		static constexpr auto meta_id() { return ConfigValue::meta_id(); }
		void view_resize() { this->_view_resize(meta_size()); }

		std::string_view get_key() const { return this->template _get_string<tll_scheme_offset_ptr_t>(offset_key); }
		void set_key(std::string_view v) { return this->template _set_string<tll_scheme_offset_ptr_t>(offset_key, v); }

		std::string_view get_value() const { return this->template _get_string<tll_scheme_offset_ptr_t>(offset_value); }
		void set_value(std::string_view v) { return this->template _set_string<tll_scheme_offset_ptr_t>(offset_value, v); }
	};

	template <typename Buf>
	static binder_type<Buf> bind(Buf &buf, size_t offset = 0) { return binder_type<Buf>(tll::make_view(buf).view(offset)); }

	template <typename Buf>
	static binder_type<Buf> bind_reset(Buf &buf) { return tll::scheme::make_binder_reset<binder_type, Buf>(buf); }
};

struct ConfigEnd
{
	static constexpr size_t meta_size() { return 0; }
	static constexpr std::string_view meta_name() { return "ConfigEnd"; }
	static constexpr int meta_id() { return 30; }

	template <typename Buf>
	struct binder_type : public tll::scheme::Binder<Buf>
	{
		using tll::scheme::Binder<Buf>::Binder;

		static constexpr auto meta_size() { return ConfigEnd::meta_size(); }
		static constexpr auto meta_name() { return ConfigEnd::meta_name(); }
		static constexpr auto meta_id() { return ConfigEnd::meta_id(); }
		void view_resize() { this->_view_resize(meta_size()); }
	};

	template <typename Buf>
	static binder_type<Buf> bind(Buf &buf, size_t offset = 0) { return binder_type<Buf>(tll::make_view(buf).view(offset)); }

	template <typename Buf>
	static binder_type<Buf> bind_reset(Buf &buf) { return tll::scheme::make_binder_reset<binder_type, Buf>(buf); }
};

struct Ok
{
	static constexpr size_t meta_size() { return 0; }
	static constexpr std::string_view meta_name() { return "Ok"; }
	static constexpr int meta_id() { return 40; }

	template <typename Buf>
	struct binder_type : public tll::scheme::Binder<Buf>
	{
		using tll::scheme::Binder<Buf>::Binder;

		static constexpr auto meta_size() { return Ok::meta_size(); }
		static constexpr auto meta_name() { return Ok::meta_name(); }
		static constexpr auto meta_id() { return Ok::meta_id(); }
		void view_resize() { this->_view_resize(meta_size()); }
	};

	template <typename Buf>
	static binder_type<Buf> bind(Buf &buf, size_t offset = 0) { return binder_type<Buf>(tll::make_view(buf).view(offset)); }

	template <typename Buf>
	static binder_type<Buf> bind_reset(Buf &buf) { return tll::scheme::make_binder_reset<binder_type, Buf>(buf); }
};

struct Error
{
	static constexpr size_t meta_size() { return 8; }
	static constexpr std::string_view meta_name() { return "Error"; }
	static constexpr int meta_id() { return 50; }
	static constexpr size_t offset_error = 0;

	template <typename Buf>
	struct binder_type : public tll::scheme::Binder<Buf>
	{
		using tll::scheme::Binder<Buf>::Binder;

		static constexpr auto meta_size() { return Error::meta_size(); }
		static constexpr auto meta_name() { return Error::meta_name(); }
		static constexpr auto meta_id() { return Error::meta_id(); }
		void view_resize() { this->_view_resize(meta_size()); }

		std::string_view get_error() const { return this->template _get_string<tll_scheme_offset_ptr_t>(offset_error); }
		void set_error(std::string_view v) { return this->template _set_string<tll_scheme_offset_ptr_t>(offset_error, v); }
	};

	template <typename Buf>
	static binder_type<Buf> bind(Buf &buf, size_t offset = 0) { return binder_type<Buf>(tll::make_view(buf).view(offset)); }

	template <typename Buf>
	static binder_type<Buf> bind_reset(Buf &buf) { return tll::scheme::make_binder_reset<binder_type, Buf>(buf); }
};

struct SetLogLevel
{
	static constexpr size_t meta_size() { return 10; }
	static constexpr std::string_view meta_name() { return "SetLogLevel"; }
	static constexpr int meta_id() { return 60; }
	static constexpr size_t offset_prefix = 0;
	static constexpr size_t offset_level = 8;
	static constexpr size_t offset_recursive = 9;

	enum class level: uint8_t
	{
		Trace = 0,
		Debug = 1,
		Info = 2,
		Warning = 3,
		Error = 4,
		Critical = 5,
	};

	enum class recursive: uint8_t
	{
		No = 0,
		Yes = 1,
	};

	template <typename Buf>
	struct binder_type : public tll::scheme::Binder<Buf>
	{
		using tll::scheme::Binder<Buf>::Binder;

		static constexpr auto meta_size() { return SetLogLevel::meta_size(); }
		static constexpr auto meta_name() { return SetLogLevel::meta_name(); }
		static constexpr auto meta_id() { return SetLogLevel::meta_id(); }
		void view_resize() { this->_view_resize(meta_size()); }

		std::string_view get_prefix() const { return this->template _get_string<tll_scheme_offset_ptr_t>(offset_prefix); }
		void set_prefix(std::string_view v) { return this->template _set_string<tll_scheme_offset_ptr_t>(offset_prefix, v); }

		using type_level = level;
		type_level get_level() const { return this->template _get_scalar<type_level>(offset_level); }
		void set_level(type_level v) { return this->template _set_scalar<type_level>(offset_level, v); }

		using type_recursive = recursive;
		type_recursive get_recursive() const { return this->template _get_scalar<type_recursive>(offset_recursive); }
		void set_recursive(type_recursive v) { return this->template _set_scalar<type_recursive>(offset_recursive, v); }
	};

	template <typename Buf>
	static binder_type<Buf> bind(Buf &buf, size_t offset = 0) { return binder_type<Buf>(tll::make_view(buf).view(offset)); }

	template <typename Buf>
	static binder_type<Buf> bind_reset(Buf &buf) { return tll::scheme::make_binder_reset<binder_type, Buf>(buf); }
};

struct Ping
{
	static constexpr size_t meta_size() { return 0; }
	static constexpr std::string_view meta_name() { return "Ping"; }
	static constexpr int meta_id() { return 70; }

	template <typename Buf>
	struct binder_type : public tll::scheme::Binder<Buf>
	{
		using tll::scheme::Binder<Buf>::Binder;

		static constexpr auto meta_size() { return Ping::meta_size(); }
		static constexpr auto meta_name() { return Ping::meta_name(); }
		static constexpr auto meta_id() { return Ping::meta_id(); }
		void view_resize() { this->_view_resize(meta_size()); }
	};

	template <typename Buf>
	static binder_type<Buf> bind(Buf &buf, size_t offset = 0) { return binder_type<Buf>(tll::make_view(buf).view(offset)); }

	template <typename Buf>
	static binder_type<Buf> bind_reset(Buf &buf) { return tll::scheme::make_binder_reset<binder_type, Buf>(buf); }
};

struct Pong
{
	static constexpr size_t meta_size() { return 0; }
	static constexpr std::string_view meta_name() { return "Pong"; }
	static constexpr int meta_id() { return 80; }

	template <typename Buf>
	struct binder_type : public tll::scheme::Binder<Buf>
	{
		using tll::scheme::Binder<Buf>::Binder;

		static constexpr auto meta_size() { return Pong::meta_size(); }
		static constexpr auto meta_name() { return Pong::meta_name(); }
		static constexpr auto meta_id() { return Pong::meta_id(); }
		void view_resize() { this->_view_resize(meta_size()); }
	};

	template <typename Buf>
	static binder_type<Buf> bind(Buf &buf, size_t offset = 0) { return binder_type<Buf>(tll::make_view(buf).view(offset)); }

	template <typename Buf>
	static binder_type<Buf> bind_reset(Buf &buf) { return tll::scheme::make_binder_reset<binder_type, Buf>(buf); }
};

struct Hello
{
	static constexpr size_t meta_size() { return 10; }
	static constexpr std::string_view meta_name() { return "Hello"; }
	static constexpr int meta_id() { return 90; }
	static constexpr size_t offset_version = 0;
	static constexpr size_t offset_service = 2;

	template <typename Buf>
	struct binder_type : public tll::scheme::Binder<Buf>
	{
		using tll::scheme::Binder<Buf>::Binder;

		static constexpr auto meta_size() { return Hello::meta_size(); }
		static constexpr auto meta_name() { return Hello::meta_name(); }
		static constexpr auto meta_id() { return Hello::meta_id(); }
		void view_resize() { this->_view_resize(meta_size()); }

		using type_version = uint16_t;
		type_version get_version() const { return this->template _get_scalar<type_version>(offset_version); }
		void set_version(type_version v) { return this->template _set_scalar<type_version>(offset_version, v); }

		std::string_view get_service() const { return this->template _get_string<tll_scheme_offset_ptr_t>(offset_service); }
		void set_service(std::string_view v) { return this->template _set_string<tll_scheme_offset_ptr_t>(offset_service, v); }
	};

	template <typename Buf>
	static binder_type<Buf> bind(Buf &buf, size_t offset = 0) { return binder_type<Buf>(tll::make_view(buf).view(offset)); }

	template <typename Buf>
	static binder_type<Buf> bind_reset(Buf &buf) { return tll::scheme::make_binder_reset<binder_type, Buf>(buf); }
};

struct StateDump
{
	static constexpr size_t meta_size() { return 0; }
	static constexpr std::string_view meta_name() { return "StateDump"; }
	static constexpr int meta_id() { return 4096; }

	template <typename Buf>
	struct binder_type : public tll::scheme::Binder<Buf>
	{
		using tll::scheme::Binder<Buf>::Binder;

		static constexpr auto meta_size() { return StateDump::meta_size(); }
		static constexpr auto meta_name() { return StateDump::meta_name(); }
		static constexpr auto meta_id() { return StateDump::meta_id(); }
		void view_resize() { this->_view_resize(meta_size()); }
	};

	template <typename Buf>
	static binder_type<Buf> bind(Buf &buf, size_t offset = 0) { return binder_type<Buf>(tll::make_view(buf).view(offset)); }

	template <typename Buf>
	static binder_type<Buf> bind_reset(Buf &buf) { return tll::scheme::make_binder_reset<binder_type, Buf>(buf); }
};

struct StateUpdate
{
	static constexpr size_t meta_size() { return 11; }
	static constexpr std::string_view meta_name() { return "StateUpdate"; }
	static constexpr int meta_id() { return 4112; }
	static constexpr size_t offset_channel = 0;
	static constexpr size_t offset_state = 8;
	static constexpr size_t offset_flags = 9;

	enum class State: uint8_t
	{
		Closed = 0,
		Opening = 1,
		Active = 2,
		Closing = 3,
		Error = 4,
		Destroy = 5,
	};

	struct Flags: public tll::scheme::Bits<uint16_t>
	{
		using tll::scheme::Bits<uint16_t>::Bits;
		constexpr auto stage() const { return get(0, 1); };
		constexpr Flags & stage(bool v) { set(0, 1, v); return *this; };
		static std::map<std::string_view, value_type> bits_descriptor()
		{
			return {
				{ "stage", static_cast<value_type>(Bits::mask(1)) << 0 },
			};
		}
	};

	template <typename Buf>
	struct binder_type : public tll::scheme::Binder<Buf>
	{
		using tll::scheme::Binder<Buf>::Binder;

		static constexpr auto meta_size() { return StateUpdate::meta_size(); }
		static constexpr auto meta_name() { return StateUpdate::meta_name(); }
		static constexpr auto meta_id() { return StateUpdate::meta_id(); }
		void view_resize() { this->_view_resize(meta_size()); }

		std::string_view get_channel() const { return this->template _get_string<tll_scheme_offset_ptr_t>(offset_channel); }
		void set_channel(std::string_view v) { return this->template _set_string<tll_scheme_offset_ptr_t>(offset_channel, v); }

		using type_state = State;
		type_state get_state() const { return this->template _get_scalar<type_state>(offset_state); }
		void set_state(type_state v) { return this->template _set_scalar<type_state>(offset_state, v); }

		using type_flags = Flags;
		type_flags get_flags() const { return this->template _get_scalar<type_flags>(offset_flags); }
		void set_flags(type_flags v) { return this->template _set_scalar<type_flags>(offset_flags, v); }
	};

	template <typename Buf>
	static binder_type<Buf> bind(Buf &buf, size_t offset = 0) { return binder_type<Buf>(tll::make_view(buf).view(offset)); }

	template <typename Buf>
	static binder_type<Buf> bind_reset(Buf &buf) { return tll::scheme::make_binder_reset<binder_type, Buf>(buf); }
};

struct StateDumpEnd
{
	static constexpr size_t meta_size() { return 0; }
	static constexpr std::string_view meta_name() { return "StateDumpEnd"; }
	static constexpr int meta_id() { return 4128; }

	template <typename Buf>
	struct binder_type : public tll::scheme::Binder<Buf>
	{
		using tll::scheme::Binder<Buf>::Binder;

		static constexpr auto meta_size() { return StateDumpEnd::meta_size(); }
		static constexpr auto meta_name() { return StateDumpEnd::meta_name(); }
		static constexpr auto meta_id() { return StateDumpEnd::meta_id(); }
		void view_resize() { this->_view_resize(meta_size()); }
	};

	template <typename Buf>
	static binder_type<Buf> bind(Buf &buf, size_t offset = 0) { return binder_type<Buf>(tll::make_view(buf).view(offset)); }

	template <typename Buf>
	static binder_type<Buf> bind_reset(Buf &buf) { return tll::scheme::make_binder_reset<binder_type, Buf>(buf); }
};

struct Message
{
	static constexpr size_t meta_size() { return 34; }
	static constexpr std::string_view meta_name() { return "Message"; }
	static constexpr size_t offset_type = 0;
	static constexpr size_t offset_name = 2;
	static constexpr size_t offset_seq = 10;
	static constexpr size_t offset_addr = 18;
	static constexpr size_t offset_data = 26;

	enum class type: int16_t
	{
		Data = 0,
		Control = 1,
	};

	template <typename Buf>
	struct binder_type : public tll::scheme::Binder<Buf>
	{
		using tll::scheme::Binder<Buf>::Binder;

		static constexpr auto meta_size() { return Message::meta_size(); }
		static constexpr auto meta_name() { return Message::meta_name(); }
		void view_resize() { this->_view_resize(meta_size()); }

		using type_type = type;
		type_type get_type() const { return this->template _get_scalar<type_type>(offset_type); }
		void set_type(type_type v) { return this->template _set_scalar<type_type>(offset_type, v); }

		std::string_view get_name() const { return this->template _get_string<tll_scheme_offset_ptr_t>(offset_name); }
		void set_name(std::string_view v) { return this->template _set_string<tll_scheme_offset_ptr_t>(offset_name, v); }

		using type_seq = int64_t;
		type_seq get_seq() const { return this->template _get_scalar<type_seq>(offset_seq); }
		void set_seq(type_seq v) { return this->template _set_scalar<type_seq>(offset_seq, v); }

		using type_addr = uint64_t;
		type_addr get_addr() const { return this->template _get_scalar<type_addr>(offset_addr); }
		void set_addr(type_addr v) { return this->template _set_scalar<type_addr>(offset_addr, v); }

		std::string_view get_data() const { return this->template _get_string<tll_scheme_offset_ptr_t>(offset_data); }
		void set_data(std::string_view v) { return this->template _set_string<tll_scheme_offset_ptr_t>(offset_data, v); }
	};

	template <typename Buf>
	static binder_type<Buf> bind(Buf &buf, size_t offset = 0) { return binder_type<Buf>(tll::make_view(buf).view(offset)); }

	template <typename Buf>
	static binder_type<Buf> bind_reset(Buf &buf) { return tll::scheme::make_binder_reset<binder_type, Buf>(buf); }
};

struct MessageForward
{
	static constexpr size_t meta_size() { return 42; }
	static constexpr std::string_view meta_name() { return "MessageForward"; }
	static constexpr int meta_id() { return 4176; }
	static constexpr size_t offset_dest = 0;
	static constexpr size_t offset_data = 8;

	template <typename Buf>
	struct binder_type : public tll::scheme::Binder<Buf>
	{
		using tll::scheme::Binder<Buf>::Binder;

		static constexpr auto meta_size() { return MessageForward::meta_size(); }
		static constexpr auto meta_name() { return MessageForward::meta_name(); }
		static constexpr auto meta_id() { return MessageForward::meta_id(); }
		void view_resize() { this->_view_resize(meta_size()); }

		std::string_view get_dest() const { return this->template _get_string<tll_scheme_offset_ptr_t>(offset_dest); }
		void set_dest(std::string_view v) { return this->template _set_string<tll_scheme_offset_ptr_t>(offset_dest, v); }

		using type_data = Message::binder_type<Buf>;
		const type_data get_data() const { return this->template _get_binder<type_data>(offset_data); }
		type_data get_data() { return this->template _get_binder<type_data>(offset_data); }
	};

	template <typename Buf>
	static binder_type<Buf> bind(Buf &buf, size_t offset = 0) { return binder_type<Buf>(tll::make_view(buf).view(offset)); }

	template <typename Buf>
	static binder_type<Buf> bind_reset(Buf &buf) { return tll::scheme::make_binder_reset<binder_type, Buf>(buf); }
};

struct ChannelClose
{
	static constexpr size_t meta_size() { return 8; }
	static constexpr std::string_view meta_name() { return "ChannelClose"; }
	static constexpr int meta_id() { return 4192; }
	static constexpr size_t offset_channel = 0;

	template <typename Buf>
	struct binder_type : public tll::scheme::Binder<Buf>
	{
		using tll::scheme::Binder<Buf>::Binder;

		static constexpr auto meta_size() { return ChannelClose::meta_size(); }
		static constexpr auto meta_name() { return ChannelClose::meta_name(); }
		static constexpr auto meta_id() { return ChannelClose::meta_id(); }
		void view_resize() { this->_view_resize(meta_size()); }

		std::string_view get_channel() const { return this->template _get_string<tll_scheme_offset_ptr_t>(offset_channel); }
		void set_channel(std::string_view v) { return this->template _set_string<tll_scheme_offset_ptr_t>(offset_channel, v); }
	};

	template <typename Buf>
	static binder_type<Buf> bind(Buf &buf, size_t offset = 0) { return binder_type<Buf>(tll::make_view(buf).view(offset)); }

	template <typename Buf>
	static binder_type<Buf> bind_reset(Buf &buf) { return tll::scheme::make_binder_reset<binder_type, Buf>(buf); }
};

} // namespace control_scheme

template <>
struct tll::conv::dump<control_scheme::Version> : public to_string_from_string_buf<control_scheme::Version>
{
	template <typename Buf>
	static inline std::string_view to_string_buf(const control_scheme::Version &v, Buf &buf)
	{
		switch (v) {
		case control_scheme::Version::Current: return "Current";
		default: break;
		}
		return tll::conv::to_string_buf<uint16_t, Buf>((uint16_t) v, buf);
	}
};

template <>
struct tll::conv::dump<control_scheme::SetLogLevel::level> : public to_string_from_string_buf<control_scheme::SetLogLevel::level>
{
	template <typename Buf>
	static inline std::string_view to_string_buf(const control_scheme::SetLogLevel::level &v, Buf &buf)
	{
		switch (v) {
		case control_scheme::SetLogLevel::level::Critical: return "Critical";
		case control_scheme::SetLogLevel::level::Debug: return "Debug";
		case control_scheme::SetLogLevel::level::Error: return "Error";
		case control_scheme::SetLogLevel::level::Info: return "Info";
		case control_scheme::SetLogLevel::level::Trace: return "Trace";
		case control_scheme::SetLogLevel::level::Warning: return "Warning";
		default: break;
		}
		return tll::conv::to_string_buf<uint8_t, Buf>((uint8_t) v, buf);
	}
};

template <>
struct tll::conv::dump<control_scheme::SetLogLevel::recursive> : public to_string_from_string_buf<control_scheme::SetLogLevel::recursive>
{
	template <typename Buf>
	static inline std::string_view to_string_buf(const control_scheme::SetLogLevel::recursive &v, Buf &buf)
	{
		switch (v) {
		case control_scheme::SetLogLevel::recursive::No: return "No";
		case control_scheme::SetLogLevel::recursive::Yes: return "Yes";
		default: break;
		}
		return tll::conv::to_string_buf<uint8_t, Buf>((uint8_t) v, buf);
	}
};

template <>
struct tll::conv::dump<control_scheme::StateUpdate::State> : public to_string_from_string_buf<control_scheme::StateUpdate::State>
{
	template <typename Buf>
	static inline std::string_view to_string_buf(const control_scheme::StateUpdate::State &v, Buf &buf)
	{
		switch (v) {
		case control_scheme::StateUpdate::State::Active: return "Active";
		case control_scheme::StateUpdate::State::Closed: return "Closed";
		case control_scheme::StateUpdate::State::Closing: return "Closing";
		case control_scheme::StateUpdate::State::Destroy: return "Destroy";
		case control_scheme::StateUpdate::State::Error: return "Error";
		case control_scheme::StateUpdate::State::Opening: return "Opening";
		default: break;
		}
		return tll::conv::to_string_buf<uint8_t, Buf>((uint8_t) v, buf);
	}
};

template <>
struct tll::conv::dump<control_scheme::Message::type> : public to_string_from_string_buf<control_scheme::Message::type>
{
	template <typename Buf>
	static inline std::string_view to_string_buf(const control_scheme::Message::type &v, Buf &buf)
	{
		switch (v) {
		case control_scheme::Message::type::Control: return "Control";
		case control_scheme::Message::type::Data: return "Data";
		default: break;
		}
		return tll::conv::to_string_buf<int16_t, Buf>((int16_t) v, buf);
	}
};
