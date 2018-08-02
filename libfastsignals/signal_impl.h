#pragma once

#include "Function_detail.h"
#include "spin_mutex.h"
#include <memory>
#include <vector>

namespace is::signals::detail
{

class signal_impl
{
public:
	uint64_t add(packed_function fn);

	void remove(uint64_t id) noexcept;

	void remove_all() noexcept;

	template <class Combiner, class Result, class Signature, class... Args>
	Result invoke(Args... args) const
	{
		// TODO: (feature) add result combiners

		packed_function slot;
		size_t slotIndex = static_cast<size_t>(-1);
		uint64_t slotId = 0;

		if constexpr (std::is_same_v<Result, void>)
		{
			while (get_next_slot(slot, slotIndex, slotId))
			{
				slot.get<Signature>()(args...);
			}
		}
		else
		{
			Combiner combiner;
			while (get_next_slot(slot, slotIndex, slotId))
			{
				if (!combiner(slot.get<Signature>()(args...)))
				{
					break;
				}
			}
			return combiner.get_value();
		}
	}

private:
	bool get_next_slot(packed_function& slot, size_t& expectedIndex, uint64_t& nextId) const;

	mutable spin_mutex m_mutex;
	std::vector<packed_function> m_functions;
	std::vector<uint64_t> m_ids;
	uint64_t m_nextId = 0;
};

using signal_impl_ptr = std::shared_ptr<signal_impl>;
using signal_impl_weak_ptr = std::weak_ptr<signal_impl>;

} // namespace is::signals::detail
