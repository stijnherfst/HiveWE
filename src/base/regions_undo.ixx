export module RegionsUndo;

import std;
import Regions;
import WorldUndoManager;

export class RegionAddAction final : public WorldCommand {
  public:
	std::vector<Region> regions;

	void undo(WorldEditContext& ctx) override {
		if (ctx.brush) {
			ctx.brush->clear_selection();
		}

		std::erase_if(ctx.regions.regions, [&](const Region& region) {
			return std::ranges::any_of(regions, [&](const Region& i) { return i.creation_number == region.creation_number; });
		});
	}

	void redo(WorldEditContext& ctx) override {
		if (ctx.brush) {
			ctx.brush->clear_selection();
		}

		ctx.regions.regions.insert(ctx.regions.regions.end(), regions.begin(), regions.end());
	}
};

export class RegionDeleteAction final : public WorldCommand {
  public:
	/// The deleted regions paired with the (ascending) indices they had,
	/// so undo restores them at their original draw order
	std::vector<std::pair<size_t, Region>> regions;

	void undo(WorldEditContext& ctx) override {
		if (ctx.brush) {
			ctx.brush->clear_selection();
		}

		for (const auto& [index, region] : regions) {
			ctx.regions.regions.insert(ctx.regions.regions.begin() + std::min(index, ctx.regions.regions.size()), region);
		}
	}

	void redo(WorldEditContext& ctx) override {
		if (ctx.brush) {
			ctx.brush->clear_selection();
		}

		std::erase_if(ctx.regions.regions, [&](const Region& region) {
			return std::ranges::any_of(regions, [&](const auto& i) { return i.second.creation_number == region.creation_number; });
		});
	}
};

export class RegionStateAction final : public WorldCommand {
  public:
	std::vector<Region> old_regions;
	std::vector<Region> new_regions;

	void undo(WorldEditContext& ctx) override {
		for (const auto& i : old_regions) {
			for (auto& j : ctx.regions.regions) {
				if (i.creation_number == j.creation_number) {
					j = i;
				}
			}
		}

		notify(ctx);
	}

	void redo(WorldEditContext& ctx) override {
		for (const auto& i : new_regions) {
			for (auto& j : ctx.regions.regions) {
				if (i.creation_number == j.creation_number) {
					j = i;
				}
			}
		}

		notify(ctx);
	}

  private:
	/// The regions were modified in place so the selection still holds, but the palette
	/// listens to this signal to refresh the names/colors it displays
	static void notify(WorldEditContext& ctx) {
		if (ctx.brush) {
			ctx.brush->selection_changed();
		}
	}
};
