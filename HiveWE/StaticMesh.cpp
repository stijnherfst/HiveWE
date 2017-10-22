#include "stdafx.h"

void StaticMesh::load(const std::string& path) {
	auto t = fs::path(path).extension();
	if (fs::path(path).extension() == ".mdx" || fs::path(path).extension() == ".MDX") {
		mdx::MDX model = mdx::MDX(path);

	} else {
		
	}
}

void StaticMesh::unload() {

}