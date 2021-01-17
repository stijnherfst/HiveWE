#pragma once

// HiveWE does not use the war3map.imp file at all, but the old WE does
// That's why we write the file at save time

class Imports {
public:
	void save() const;
};