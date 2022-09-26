#pragma once
#include "imgui.h"

#include <filesystem>
#include <optional>
#include <string>

class ContentBrowserPanel {
public:
	ContentBrowserPanel();

	void OnImGuiRender(bool* pOpen);
private:
	void DrawTree();
	void DrawTreeRecursive(const std::filesystem::path& currentPath);
	void DrawContent();
private:
	std::filesystem::path mCurrentDirectory;
	std::optional<std::filesystem::path> mSelectedDirectory;

	std::wstring mModelsPath = L"C:\\Users\\Lenovo\\Desktop\\EngineZeroOne\\Models";
};