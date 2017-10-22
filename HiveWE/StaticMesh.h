#pragma once

class StaticMesh : public Resource {
public:
	StaticMesh(const std::string& path) : Resource(path) {
		load(path);
	}

protected:
	void load(const std::string& path);

	void unload();
};