module;

#include <glm/glm.hpp>

#include <outcome/outcome.hpp>
#include <outcome/try.hpp>

module MDX;

import std;

namespace outcome = OUTCOME_V2_NAMESPACE;
using OUTCOME_V2_NAMESPACE::failure;
using OUTCOME_V2_NAMESPACE::result;

// Experimental implementation (i.e. garbage code)
// Rewrite when pattern matching is added to C++
// Rewrite when ? operator is added to C++

namespace mdx {
#define TRY(r)                                \
	{                                         \
		if (auto optional = r; optional) {    \
			return failure(optional.value()); \
		}                                     \
	}

	struct MDLReader {
		std::vector<std::string_view> tokens;
		size_t position = 0;

		void advance() {
			position += 1;
		}

		result<std::string_view, std::string> consume() {
			if (position >= tokens.size()) [[unlikely]] {
				return failure("Expected a token but reached the end of the file");
			}
			auto token = tokens[position];
			position += 1;
			return outcome::success(token);
		}

		std::string_view current() {
			return tokens[position];
		}

		result<std::string_view, std::string> peek() {
			if (position >= tokens.size()) [[unlikely]] {
				return failure("Expected a token but reached the end of the file");
			}
			return outcome::success(tokens[position]);
		}

		std::optional<std::string> consume(std::string_view token) {
			if (position >= tokens.size()) [[unlikely]] {
				return std::format("Expected \"{}\", but reached the end of the file", token);
			}
			if (tokens[position] != token) [[unlikely]] {
				return std::format("Expected \"{}\" but got \"{}\"", token, tokens[position]);
			}
			position += 1;
			return std::nullopt;
		}

		result<int64_t, std::string> consume_integer() {
			if (position >= tokens.size()) [[unlikely]] {
				return std::format("Expected an integer, but reached the end of the file");
			}

			int64_t integer;
			std::from_chars(tokens[position].data(), tokens[position].data() + tokens[position].size(), integer); // ToDo handle parsing error
			position += 1;
			return outcome::success(integer);
		}

		result<float, std::string> consume_float() {
			if (position >= tokens.size()) [[unlikely]] {
				return std::format("Expected an integer, but reached the end of the file");
			}

			float number;
			std::from_chars(tokens[position].data(), tokens[position].data() + tokens[position].size(), number); // ToDo handle parsing error
			position += 1;
			return outcome::success(number);
		}

		result<glm::vec2, std::string> consume_vec2() {
			TRY(consume("{"));
			glm::vec2 vec;
			OUTCOME_TRY(vec.x, consume_float());
			TRY(consume(","));
			OUTCOME_TRY(vec.y, consume_float());
			TRY(consume("}"));
			return vec;
		}

		result<glm::vec3, std::string> consume_vec3() {
			TRY(consume("{"));
			glm::vec3 vec;
			OUTCOME_TRY(vec.x, consume_float());
			TRY(consume(","));
			OUTCOME_TRY(vec.y, consume_float());
			TRY(consume(","));
			OUTCOME_TRY(vec.z, consume_float());
			TRY(consume("}"));
			return vec;
		}

		result<glm::vec4, std::string> consume_vec4() {
			TRY(consume("{"));
			glm::vec4 vec;
			OUTCOME_TRY(vec.x, consume_float());
			TRY(consume(","));
			OUTCOME_TRY(vec.y, consume_float());
			TRY(consume(","));
			OUTCOME_TRY(vec.z, consume_float());
			TRY(consume(","));
			OUTCOME_TRY(vec.w, consume_float());
			TRY(consume("}"));
			return vec;
		}

		result<std::string_view, std::string> consume_quoted_string() {
			const auto token = current();
			if (position >= tokens.size()) [[unlikely]] {
				return failure(std::format("Expected quoted string (e.g. \"SomeName\"), but reached the end of the file"));
			}

			if (token.size() < 3 || token.front() != '\"' || token.back() != '\"') [[unlikely]] {
				return failure(std::format("The name should be surrounded in quotes \"likethis\" and at least one character long, but is \"{}\"", token));
			}

			position += 1;
			return outcome::success(token.substr(1, token.size() - 2)); // Get rid of quotes
		}
	};

	outcome::result<void, std::string> parse_version_chunk(MDLReader& reader, MDX& mdx) {
		TRY(reader.consume("Version"));
		TRY(reader.consume("{"));
		TRY(reader.consume("FormatVersion"));

		OUTCOME_TRY(mdx.version, reader.consume_integer());
		if (mdx.version != 800 || mdx.version != 900 || mdx.version != 1000) [[unlikely]] {
			return std::format("Invalid version {}, expected 800, 900, 1000", mdx.version);
		}

		TRY(reader.consume(","));
		TRY(reader.consume("}"));
		return outcome::success();
	}

	outcome::result<void, std::string> parse_model_chunk(MDLReader& reader, MDX& mdx) {
		TRY(reader.consume("Model"));

		OUTCOME_TRY(mdx.name, reader.consume_quoted_string());

		TRY(reader.consume("{"));

		while (true) {
			OUTCOME_TRY(auto token, reader.consume());
			if (token == "}") {
				break;
			}

			if (token == "BlendTime") {
				OUTCOME_TRY(mdx.blend_time, reader.consume_integer());
			}

			if (token == "MinimumExtent") {
				OUTCOME_TRY(mdx.extent.minimum, reader.consume_vec3());
			}

			if (token == "MaximumExtent") {
				OUTCOME_TRY(mdx.extent.minimum, reader.consume_vec3());
			}

			if (token == "BoundsRadius") {
				OUTCOME_TRY(mdx.extent.bounds_radius, reader.consume_float());
			}
			TRY(reader.consume(","));
		}

		return outcome::success();
	}

	outcome::result<void, std::string> parse_sequences_chunk(MDLReader& reader, MDX& mdx) {
		TRY(reader.consume("Sequences"));

		OUTCOME_TRY(auto sequence_count, reader.consume_integer());

		TRY(reader.consume("{"));

		for (size_t i = 0; i < sequence_count; i++) {
			TRY(reader.consume("Anim"));
			OUTCOME_TRY(auto name, reader.consume_quoted_string());
			TRY(reader.consume("{"));

			Sequence sequence;
			while (true) {
				OUTCOME_TRY(auto token, reader.consume());
				if (token == "}") {
					break;
				}

				if (token == "NonLooping") {
					sequence.flags |= Sequence::non_looping;
				}

				if (token == "MinimumExtent") {
					OUTCOME_TRY(sequence.extent.minimum, reader.consume_vec3());
				}

				if (token == "MaximumExtent") {
					OUTCOME_TRY(sequence.extent.maximum, reader.consume_vec3());
				}

				if (token == "BoundsRadius") {
					OUTCOME_TRY(sequence.extent.bounds_radius, reader.consume_float());
				}
				TRY(reader.consume(","));
			}
			mdx.sequences.push_back(sequence);
		}

		return outcome::success();
	}

	outcome::result<void, std::string> parse_global_sequences(MDLReader& reader, MDX& mdx) {
		TRY(reader.consume("GlobalSequences"));
		OUTCOME_TRY(auto global_sequence_count, reader.consume_integer());
		TRY(reader.consume("{"));

		
		for (size_t i = 0; i < global_sequence_count; i++) {
			TRY(reader.consume("Duration"));
			
			uint32_t duration;
			OUTCOME_TRY(duration, reader.consume_integer());
			mdx.global_sequences.push_back(duration);
		}

		return outcome::success();
	}

	outcome::result<void, std::string> parse_textures(MDLReader& reader, MDX& mdx) {
		TRY(reader.consume("Textures"));
		OUTCOME_TRY(auto texture_count, reader.consume_integer());
		TRY(reader.consume("{"));

		for (size_t i = 0; i < texture_count; i++) {
			TRY(reader.consume("Bitmap"));
			TRY(reader.consume("{"));

			Texture texture;
			while (true) {
				OUTCOME_TRY(auto token, reader.consume());
				if (token == "}") {
					break;
				}

				if (token == "Image") {
					OUTCOME_TRY(texture.file_name, reader.consume_quoted_string());
				}

				if (token == "ReplaceableId") {
					OUTCOME_TRY(texture.replaceable_id, reader.consume_integer());
				}
				TRY(reader.consume(","));
			}
			mdx.textures.push_back(texture);
		}

		return outcome::success();
	}

	outcome::result<void, std::string> parse_materials(MDLReader& reader, MDX& mdx) {
		TRY(reader.consume("Materials"));
		OUTCOME_TRY(auto material_count, reader.consume_integer());
		TRY(reader.consume("{"));

		for (size_t i = 0; i < material_count; i++) {
			TRY(reader.consume("Material"));
			TRY(reader.consume("{"));

			Material material;
			while (true) {
				OUTCOME_TRY(auto token, reader.consume());
				if (token == "}") {
					break;
				}

				if (token == "Shader") {
					OUTCOME_TRY(auto yeet, reader.consume_quoted_string());
				}

				if (token == "Layer") {
					TRY(reader.consume("{"));
					Layer layer;
					while (true) {
						OUTCOME_TRY(auto token, reader.consume());
						if (token == "}") {
							break;
						}

						if (token == "FilterMode") {
							OUTCOME_TRY(auto mode, reader.consume_quoted_string());
							if (mode == "Additive") {
								layer.blend_mode = 3;
							}
						}

						if (token == "Unshaded") {
							layer.shading_flags |= Layer::ShadingFlags::unshaded;
						}

						if (token == "Unfogged") {
							layer.shading_flags |= Layer::ShadingFlags::unfogged;
						}

						if (token == "static") {
							OUTCOME_TRY(auto token, reader.consume());
							if (token == "TextureID") {
								OUTCOME_TRY(layer.texture_animation_id, reader.consume_integer());
							}
							if (token == "Alpha") {
								OUTCOME_TRY(layer.alpha, reader.consume_float());
							}
						}
						TRY(reader.consume(","));
					}
					material.layers.push_back(layer);
				}
				TRY(reader.consume(","));
			}
			mdx.materials.push_back(material);
		}

		return outcome::success();
	}

	outcome::result<void, std::string> parse_geoset(MDLReader& reader, MDX& mdx) {
		TRY(reader.consume("Geoset"));
		TRY(reader.consume("{"));

		Geoset geoset;

		TRY(reader.consume("Vertices"));
		OUTCOME_TRY(auto vertex_count, reader.consume_integer());
		TRY(reader.consume("{"));

		for (size_t i = 0; i < vertex_count; i++) {
			OUTCOME_TRY(auto vertex, reader.consume_vec3());
			TRY(reader.consume(","));
			geoset.vertices.push_back(vertex);
		}
		TRY(reader.consume("}"));

		TRY(reader.consume("Normals"));
		OUTCOME_TRY(auto normal_count, reader.consume_integer());
		TRY(reader.consume("{"));

		for (size_t i = 0; i < normal_count; i++) {
			OUTCOME_TRY(auto normal, reader.consume_vec3());
			TRY(reader.consume(","));
			geoset.normals.push_back(normal);
		}
		TRY(reader.consume("}"));

		OUTCOME_TRY(auto is_tangents, reader.peek());
		if (is_tangents == "Tangents") {
			TRY(reader.consume("Tangents"));
			OUTCOME_TRY(auto tangent_count, reader.consume_integer());
			TRY(reader.consume("{"));

			for (size_t i = 0; i < tangent_count; i++) {
				OUTCOME_TRY(auto tangent, reader.consume_vec4());
				geoset.tangents.push_back(tangent);
				TRY(reader.consume(","));
			}
			TRY(reader.consume("}"));
		}

		TRY(reader.consume("TVertices"));
		OUTCOME_TRY(auto uv_count, reader.consume_integer());
		TRY(reader.consume("{"));

		std::vector<glm::vec2> uvs;
		for (size_t i = 0; i < uv_count; i++) {
			OUTCOME_TRY(auto uv, reader.consume_vec2());
			uvs.push_back(uv);
			TRY(reader.consume(","));
		}
		geoset.uv_sets.push_back(uvs);
		TRY(reader.consume("}"));

		OUTCOME_TRY(auto is_skin_weights, reader.peek());
		if (is_skin_weights == "SkinWeights") {
			TRY(reader.consume("SkinWeights"));
			OUTCOME_TRY(auto skin_count, reader.consume_integer());
			TRY(reader.consume("{"));

			for (size_t i = 0; i < skin_count * 8; i++) {
				OUTCOME_TRY(auto weight, reader.consume_integer());
				geoset.skin.push_back(weight);
				TRY(reader.consume(","));
			}
			TRY(reader.consume("}"));
		}

		TRY(reader.consume("VertexGroup"));
		TRY(reader.consume("{"));

		for (size_t i = 0; i < uv_count; i++) {
			OUTCOME_TRY(auto vertex_group, reader.consume_integer());
			geoset.vertex_groups.push_back(vertex_group);
			TRY(reader.consume(","));
		}

		TRY(reader.consume("}"));

		TRY(reader.consume("Faces"));
		OUTCOME_TRY(auto face_count, reader.consume_integer());
		assert(face_count == 1); // I don't even know if you can have multiple

		OUTCOME_TRY(auto index_count, reader.consume_integer());
		TRY(reader.consume("{"));

		for (size_t i = 0; i < face_count; i++) {
			TRY(reader.consume("Triangles")); // We only support triangles
			TRY(reader.consume("{"));

			for (size_t j = 0; j < index_count; j++) {
				OUTCOME_TRY(auto index, reader.consume_integer());
				geoset.faces.push_back(index);
				if (i < face_count - 1) {
					TRY(reader.consume(","));
				}
			}

			TRY(reader.consume("}"));
		}
		TRY(reader.consume("}"));

		TRY(reader.consume("Groups"));
		OUTCOME_TRY(auto group_count, reader.consume_integer());
		OUTCOME_TRY(auto dunno, reader.consume_integer());
		TRY(reader.consume("{"));

		for (size_t i = 0; i < group_count; i++) {
			TRY(reader.consume("Matrices"));
			TRY(reader.consume("{"));

			OUTCOME_TRY(auto matrix, reader.consume_integer());
			geoset.matrix_groups.push_back(matrix); // ?

			TRY(reader.consume("}"));
		}
		TRY(reader.consume("}"));

		//TRY(reader.consume("MinimumExtent"));
		//OUTCOME_TRY(geoset.extent.minimum, reader.consume_vec3());
		//TRY(reader.consume(","));
		//TRY(reader.consume("MaximumExtent"));
		//OUTCOME_TRY(geoset.extent.maximum, reader.consume_vec3());
		//TRY(reader.consume(","));
		//TRY(reader.consume("BoundsRadius"));
		//OUTCOME_TRY(geoset.extent.bounds_radius, reader.consume_float());
		//TRY(reader.consume(","));

		TRY(reader.consume("MaterialID"));
		OUTCOME_TRY(geoset.material_id, reader.consume_integer());
		TRY(reader.consume(","));

		TRY(reader.consume("SelectionGroup"));
		OUTCOME_TRY(geoset.selection_group, reader.consume_integer());
		TRY(reader.consume(","));

		TRY(reader.consume("LevelOfDetail"));
		OUTCOME_TRY(geoset.lod, reader.consume_integer());
		TRY(reader.consume(","));

		TRY(reader.consume("Name"));
		OUTCOME_TRY(geoset.lod_name, reader.consume_quoted_string());
		TRY(reader.consume(","));

		mdx.geosets.push_back(geoset);

		return outcome::success();
	}

	outcome::result<void, std::string> parse_geoset_anim(MDLReader& reader, MDX& mdx) {
		TRY(reader.consume("GeosetAnim"));
		TRY(reader.consume("{"));

		TRY(reader.consume("Alpha"));
		OUTCOME_TRY(auto alpha_count, reader.consume_integer());

		for (size_t i = 0; i < alpha_count; i++) {
			TRY(reader.consume("Bitmap"));
			TRY(reader.consume("{"));

			Texture texture;
			while (true) {
				OUTCOME_TRY(auto token, reader.consume());
				if (token == "}") {
					break;
				}

				if (token == "Image") {
					OUTCOME_TRY(texture.file_name, reader.consume_quoted_string());
				}

				if (token == "ReplaceableId") {
					OUTCOME_TRY(texture.replaceable_id, reader.consume_integer());
				}
				TRY(reader.consume(","));
			}
			mdx.textures.push_back(texture);
		}

		return outcome::success();
	}

	result<MDX, std::string> MDX::from_mdl(std::string_view mdl) {
		MDLReader reader;

		// Tokenize
		while (mdl.size() > 1) {
			size_t pos = mdl.find_first_of(" \t\r\n,");

			if (mdl[0] == '"') {
				pos = mdl.find_first_of('\"', 1) + 1;
			}
			reader.tokens.emplace_back(mdl.substr(0, pos));

			std::println("{}", mdl.substr(0, pos));

			pos = mdl.find_first_not_of(" \t\r\n,", pos);
			if (pos == std::string::npos) {
				break;
			}
			mdl.remove_prefix(pos);
		}

		MDX mdx;
		OUTCOME_TRY(parse_version_chunk(reader, mdx));

		while (!reader.tokens.empty()) {
			if (reader.current() == "Model") {
				OUTCOME_TRY(parse_model_chunk(reader, mdx));
			}

			if (reader.current() == "Sequences") {
				OUTCOME_TRY(parse_sequences_chunk(reader, mdx));
			}

			if (reader.current() == "GlobalSequences") {
				OUTCOME_TRY(parse_global_sequences(reader, mdx));
			}

			if (reader.current() == "Textures") {
				OUTCOME_TRY(parse_textures(reader, mdx));
			}

			if (reader.current() == "Material") {
				OUTCOME_TRY(parse_materials(reader, mdx));
			}

			if (reader.current() == "Geoset") {
				OUTCOME_TRY(parse_geoset(reader, mdx));
			}

			if (reader.current() == "GeosetAnim") {
				OUTCOME_TRY(parse_geoset_anim(reader, mdx));
			}

			return failure(std::format("Error reading token {}, expected a chunk type (Version/Model/Textures/etc). Make sure to match the casing", reader.current()));
		}

		return mdx;
	}
} // namespace mdx