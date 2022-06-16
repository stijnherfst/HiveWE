#pragma once

import ResourceManager;

#include <filesystem>
namespace fs = std::filesystem;

#include <vector>

#include <QIcon>

class QIconResource : public Resource {
public:
	QIcon icon;

	static constexpr const char* name = "QIconResource";

	explicit QIconResource() = default;
	explicit QIconResource(const fs::path& path);
};