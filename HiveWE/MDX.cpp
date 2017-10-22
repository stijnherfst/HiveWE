#include "stdafx.h"



namespace mdx {
	MDX::MDX(std::string path) {
		load(path);
	}

	void MDX::load(const std::string& path) {
		BinaryReader reader(hierarchy.open_file(path));

		std::string magic_number = reader.readString(4);
		if (magic_number != "MDLX") {
			std::cout << "The files magic number is incorrect. Should be MDLX, is: " << magic_number << std::endl;
			return;
		}

		std::string header = "Start";
		uint32_t size;
		
		while (header != "") {
			header = reader.readString(4);
			size = reader.read<uint32_t>();

			//if (header == "GEOS") {
			//	std::cout << "Geos" << std::endl;
			//} else if (header == )
		}

	}
}