#pragma once

#include <filesystem>
namespace fs = std::filesystem;

import ResourceManager;

#include <vector>

#include <QIcon>

class QIconResource : public Resource {
public:
	QIcon icon;

	static constexpr const char* name = "QIconResource";

	explicit QIconResource() = default;
	explicit QIconResource(const fs::path& path);
};