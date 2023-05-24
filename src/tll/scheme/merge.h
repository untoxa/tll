#ifndef _TLL_SCHEME_MERGE_H
#define _TLL_SCHEME_MERGE_H

#include <tll/scheme.h>
#include <tll/util/listiter.h>
#include <tll/util/result.h>

#include <list>
#include <set>

namespace tll::scheme::_ {

template <typename T>
T * lookup(T * list, std::string_view name)
{
	for (auto ptr = list; ptr; ptr = ptr->next) {
		if (ptr->name == name)
			return ptr;
	}
	return nullptr;
}

template <typename T>
T ** find_tail(T ** list)
{
	if (!list)
		return nullptr;
	for (; *list; list = &(*list)->next) {}
	return list;
}

inline bool compare(const tll::scheme::Message * lhs, const tll::scheme::Message * rhs);
inline bool compare(const tll::scheme::Field * lhs, const tll::scheme::Field * rhs)
{
	if (lhs->type != rhs->type)
		return false;
	if (lhs->size != rhs->size)
		return false;
	using Field = tll::scheme::Field;
	switch (lhs->type) {
	case Field::Message:
		return compare(lhs->type_msg, rhs->type_msg);
	case Field::Array:
		return compare(lhs->type_array, rhs->type_array);
	case Field::Pointer:
		return compare(lhs->type_ptr, rhs->type_ptr);
	default:
		break;
	}
	return true;
}

inline bool compare(const tll::scheme::Message * lhs, const tll::scheme::Message * rhs)
{
	if (std::string_view(lhs->name) != rhs->name)
		return false;
	if (lhs->msgid != rhs->msgid)
		return false;
	if (lhs->size != rhs->size)
		return false;
	for (auto lf = lhs->fields, rf = rhs->fields; lf && rf; lf = lf->next, rf = rf->next) {
		if (!compare(lf, rf))
			return false;
	}
	return true;
}

inline void depends(const tll::scheme::Field * f, std::set<const tll::scheme::Message *> &deps);
inline void depends(const tll::scheme::Union * u, std::set<const tll::scheme::Message *> &deps)
{
	for (auto & f : tll::util::list_wrap(u->fields)) {
		depends(&f, deps);
	}
}

inline void depends(const tll::scheme::Message * msg, std::set<const tll::scheme::Message *> &deps)
{
	deps.insert(msg);
	for (auto & f : tll::util::list_wrap(msg->fields)) {
		depends(&f, deps);
	}
}

inline void depends(const tll::scheme::Field * f, std::set<const tll::scheme::Message *> &deps)
{
	using Field = tll::scheme::Field;
	switch (f->type) {
	case Field::Message:
		return depends(f->type_msg, deps);
	case Field::Pointer:
		return depends(f->type_ptr, deps);
	case Field::Array:
		return depends(f->type_array, deps);
	case Field::Union:
		return depends(f->type_union, deps);
	default:
		break;
	}
}

} // namespace tll::scheme::_

namespace tll::scheme {
inline tll::result_t<tll::Scheme *> merge(const std::list<const tll::Scheme *> &list)
{
	using namespace tll::scheme::_;
	std::unique_ptr<tll::Scheme> result;

	for (auto scheme : list) {
		if (!scheme)
			continue;
		std::unique_ptr<tll::Scheme> tmp { scheme->copy() };
		if (!result) {
			result.reset(tmp.release());
			continue;
		}

		for (auto & i : tll::util::list_wrap(tmp->enums)) {
			if (auto r = lookup(result->enums, i.name); r)
				return tll::error(fmt::format("Duplicate global enum {}", i.name));
		}

		std::swap(*find_tail(&result->enums), tmp->enums);

		for (auto & i : tll::util::list_wrap(tmp->unions)) {
			if (auto r = lookup(result->unions, i.name); r)
				return tll::error(fmt::format("Duplicate global union {}", i.name));
		}

		std::swap(*find_tail(&result->unions), tmp->unions);

		std::set<const tll::scheme::Message *> move;

		for (auto & m : tll::util::list_wrap(tmp->messages)) {
			if (!m.msgid)
				continue;
			if (auto r = result->lookup(m.name); r) {
				if (!compare(&m, r))
					return tll::error(fmt::format("Non-matching message {} {}", m.name, r->name));
				continue;
			} else if (auto r = result->lookup(m.msgid); r)
				return tll::error(fmt::format("Duplicate msgid {}: {} and {}", m.msgid, r->name, m.name));

			move.insert(&m);
			depends(&m, move);
		}

		auto ptr = &tmp->messages;
		while (*ptr) {
			if (move.find(*ptr) != move.end()) {
				auto m = *ptr;
				*ptr = m->next;
				m->next = nullptr;
				*find_tail(&result->messages) = m;
			} else
				ptr = &(*ptr)->next;
		}
	}


	return result.release();
}
} // namespace tll::scheme

#endif//_TLL_SCHEME_MERGE_H
