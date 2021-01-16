#pragma once

#include <string>
#include <vector>

#include "BinaryReader.h"

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
	float pitch_variance;
	int priority;
	int channel;
	float min_distance;
	float max_distance;
	float distance_cutoff;
	float cone_inside;
	float cone_outside;
	int cone_outside_volume;
	float cone_orientation_x;
	float cone_orientation_y;
	float cone_orientation_z;
};

class Sounds {
public:
	std::vector<Sound> sounds;

	void load();
	void save() const;
};