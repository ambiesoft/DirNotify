#pragma once
class PopMessage
{
	std::wstring firstLine_;
	std::wstring secondLine_;

public:
	PopMessage(const std::wstring& firstLine, const std::wstring& secondLine) :
		firstLine_(firstLine), secondLine_(secondLine) {}

	std::wstring getFirstLine() const {
		return firstLine_;
	}
	std::wstring getSecondLine() const {
		return secondLine_;
	}
	std::wstring ToString() const {
		return firstLine_ + L"\r\n" + secondLine_;
	}
};
