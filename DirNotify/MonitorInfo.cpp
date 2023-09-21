#include "stdafx.h"
#include "DirNotify.h"

#include "MonitorInfo.h"

using namespace Ambiesoft;

std::wstring MonitorInfo::getMonitarTagetAsString() const {
	if (isMonitorFile()) {
		DASSERT(!isMonitorDirectory());
		return I18N(L"File");
	}
	if (isMonitorDirectory()) {
		return I18N(L"Directory");
	}
	return I18N(L"Unknown");
}