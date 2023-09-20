#pragma once
#include "stdafx.h"
#include "DirNotify.h"
#include "NotifyPod.h"

using namespace Ambiesoft;
using namespace Ambiesoft::stdosd;

bool NotifyPod::refinePods() {
	notifies_ = stdUniqueVector(notifies_);
	if (notifies_.size() == 0)
		return false;

	return true;
}
