#pragma once

struct Sound {
	std::string name;
	std::string file;
	std::string eax_effect;
	//int flags;
	bool looping;
	bool is_3d;
	bool stop_out_of_range;
	bool music;

	int fade_in_rate;
	int fade_out_rate;
	int volume;
	float pitch;
	float unknown;
	int unknown2;
	int channel;
	float min_distance;
	float max_distance;
	float distance_cutoff;
	float unknown3;
	float unknown4;
	int unknown5;
	float unknown6;
	float unknown7;
	float unknown8;
};

class Sounds {
public:
	std::vector<Sound> sounds;

	void load(BinaryReader& reader);
	void save() const;
};