#pragma once
class MonitorInfo
{
	bool monitorFile_;
	bool monitorDir_;
	bool monitorSub_;
	std::wstring dir_;

public:
	MonitorInfo(const std::wstring& dir, bool monitorFile, bool monitorDir, bool monitorSub):
		dir_(dir), monitorFile_(monitorFile), monitorDir_(monitorDir), monitorSub_(monitorSub){}

	std::wstring getDir() const {
		return dir_;
	}
	std::wstring getMonitarTagetAsString() const;
	bool isSubTree() const {
		return monitorSub_;
	}
	bool isMonitorFile() const {
		return monitorFile_;
	}
	bool isMonitorDirectory() const {
		return monitorDir_;
	}
};