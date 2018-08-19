#pragma once

class DoodadBrush : public Brush {
public:

	std::shared_ptr<StaticMesh> mesh;

	void apply() override;
	void render() const override;
};