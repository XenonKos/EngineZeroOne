#pragma once
#include <d3d12.h>
#include <comdef.h>
#include <string>

class D3D12Exception {
public:
	D3D12Exception(HRESULT hr, const std::wstring& functionName, const std::wstring& fileName, int lineNumber)
		: ErrorCode(hr),
		FunctionName(functionName),
		FileName(fileName),
		LineNumber(lineNumber) {}

	std::wstring ToString() const {
		// Get the string description of the error code.
		_com_error err(ErrorCode);
		std::wstring msg = err.ErrorMessage();

		return FunctionName + L" failed in " + FileName + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
	}

	HRESULT ErrorCode;
	std::wstring FunctionName;
	std::wstring FileName;
	int LineNumber;
};