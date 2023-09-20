#pragma once
#include "DirNotify.h"

class NotifyPod
{
	std::wstring dir_;
	std::vector<NotifyPair> notifies_;
	bool RemoveOne(const std::wstring& data) 
	{
		for (std::vector<NotifyPair>::iterator it=notifies_.begin(); it != notifies_.end(); ++it)
		{
			if (it->second == data) {
				notifies_.erase(it);
				return true;
			}
		}
		return false;
	}
public:
	NotifyPod(const std::wstring& dir, std::vector<NotifyPair>& notifies) :
		dir_(dir), notifies_(notifies) {}
	std::wstring getDir() const {
		return dir_;
	}
	size_t getCount() const {
		return notifies_.size();
	}
	DWORD getAction(size_t i) const {
		return notifies_[i].first;
	}
	std::wstring getData(size_t i) const {
		return notifies_[i].second;
	}
	bool refinePods();

	void removeAny(const std::wstring& data) {
		while (RemoveOne(data))
			;
	}
};

