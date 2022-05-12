#include "scsFileAccess.hpp"

namespace scsFileAccess
{

	_SCSEntryList&& _SCSEntryList::sublist(function<bool(pSCSEntry s)>filter)
	{
		_SCSEntryList slist;
		for (auto const& itr : *this)
			if (filter(itr))
				slist += itr;
		return move(slist);
	}

	_SCSEntryList&& _SCSEntryList::deep_clone()
	{
		_SCSEntryList clist;
		for (auto const& itr : *this)
			clist += make_shared<SCSEntry>(*itr.get());
		return move(clist);
	}

	_SCSEntryList&& _SCSEntryList::operator+(const pSCSEntry& l)
	{
		_SCSEntryList list(*this);
		list.push_back(l);
		return move(list);
	}

	_SCSEntryList&& _SCSEntryList::operator-(const pSCSEntry& l)
	{
		_SCSEntryList list;
		remove_copy(this->begin(), this->end(), back_inserter(list), l);
		return move(list);
	}

	_SCSEntryList&& _SCSEntryList::union_set(const pSCSEntry& l)
	{
		return union_set(l, default_comparator);
	}

	_SCSEntryList&& _SCSEntryList::cross_set(const pSCSEntry& l)
	{
		return cross_set(l, default_comparator);
	}

	_SCSEntryList&& _SCSEntryList::union_set(const pSCSEntry& l, function<int8_t(pSCSEntry, pSCSEntry)>comparator)
	{
		_SCSEntryList list(*this);
		for (auto const& itr : list)
			if (comparator(itr, l)==0)
				return move(list);
		return list + l;
	}

	_SCSEntryList&& _SCSEntryList::cross_set(const pSCSEntry& l, function<int8_t(pSCSEntry, pSCSEntry)>comparator)
	{
		for (auto const& itr : *this)
			if (comparator(itr, l)==0)
				return move(_SCSEntryList({ l }));
		return move(_SCSEntryList());
	}

	_SCSEntryList&& _SCSEntryList::operator+(const _SCSEntryList& l)
	{
		_SCSEntryList list(*this);
		copy(l.begin(), l.end(), back_inserter(list));
		return move(list);
	}

	_SCSEntryList&& _SCSEntryList::operator-(const _SCSEntryList& l)
	{
		auto difference = [&l](pSCSEntry s)
		{
			for (auto const& itr : l)
				if (s == itr)
					return true;
			return false;
		};
		_SCSEntryList list;
		remove_copy_if(this->begin(), this->end(), back_inserter(list), difference);
		return move(list);
	}

	_SCSEntryList&& _SCSEntryList::union_set(const _SCSEntryList& l)
	{
		return union_set(l, default_comparator);
	}

	/*_SCSEntryList&& _SCSEntryList::cross_set(const _SCSEntryList& l)
	{
		return cross_set(l, default_comparator);
	}*/

	_SCSEntryList&& _SCSEntryList::union_set(const _SCSEntryList& l, function<int8_t(pSCSEntry, pSCSEntry)> comparator)
	{
		_SCSEntryList list;
		auto itr1 = this->begin();
		auto itr2 = l.begin();
		while ( itr1 != this->end() && itr2 != l.end())
		{
			if (itr1 + 1 != this->end() && comparator(*itr1, *(itr1 + 1)) < 0 || itr2 + 1 != this->end() && comparator(*itr2, *(itr2 + 1)) <0 )
				throw SCSSException(__func__, "one or both of the lists are unsorted");
			if (comparator(*itr1, *itr2) > 0)
			{
				list += *itr1;
				itr1++;
			}
			else if (comparator(*itr1, *itr2) < 0)
			{
				list += *itr2;
				itr2++;
			}
			else
			{
				list += *itr1;
				itr1++;
				itr2++;
			}
			if (itr1 + 1 == this->end())
			{
				for (; itr2 != l.end(); itr2++)
				{
					if (itr2 + 1 != l.end() && comparator(*itr2, *(itr2 + 1)) < 0)
						throw SCSSException(__func__, "one or both of the lists are unsorted");
					list += *itr2;
				}
				return move(list);
			}
			if (itr2 + 1 == l.end())
			{
				for (; itr1 != this->end(); itr1++)
				{
					if (itr1 + 1 != this->end() && comparator(*itr1, *(itr1 + 1)) < 0)
						throw SCSSException(__func__, "one or both of the lists are unsorted");
					list += *itr1;
				}
				return move(list);
			}
		}
		return move(list);
	}

	_SCSEntryList&& _SCSEntryList::operator+(_SCSEntryList&& r)
	{
		_SCSEntryList list(*this);
		move(r.begin(), r.end(), back_inserter(list));
		return move(list);
	}

	_SCSEntryList&& _SCSEntryList::operator-(function<bool(pSCSEntry s)>filter)
	{
		_SCSEntryList slist;
		for (auto const& itr : *this)
			if (!filter(itr))
				slist += itr;
		return move(slist);
	}

	void _SCSEntryList::sort(function<bool(pSCSEntry l, pSCSEntry r)>comparator)
	{
		std::sort(this->begin(), this->end(), comparator);
	}

	void _SCSEntryList::sort(SourceType file_type)
	{
		if (file_type == SourceType::SCS)
			std::sort(this->begin(), this->end(), [](pSCSEntry l, pSCSEntry r) {return l->hashcode < r->hashcode; });
		else
			std::sort(this->begin(), this->end(), [](pSCSEntry l, pSCSEntry r) {return l->file_name < r->file_name; });
	}
}

