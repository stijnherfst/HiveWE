module MDX;

import std;
import std.compat;
import <glm/glm.hpp>;
import <outcome/outcome.hpp>;
import <outcome/try.hpp>;

namespace outcome = OUTCOME_V2_NAMESPACE;
using OUTCOME_V2_NAMESPACE::failure;
using OUTCOME_V2_NAMESPACE::result;

namespace mdx {
#define TRY(r) \
	{ \
		if (auto optional = r; optional) { \
			return failure(optional.value()); \
		} \
	}

	struct Token {
		std::string_view text;
		uint32_t line;
	};

	static result<std::vector<Token>, std::string> tokenize(const std::string_view source) {
		std::vector<Token> tokens;
		size_t pos = 0;
		uint32_t line = 1;

		auto is_punct = [](const char c) {
			return c == '{' || c == '}' || c == ':';
		};
		auto is_separator = [](const char c) {
			return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ',';
		};

		while (pos < source.size()) {
			const char c = source[pos];

			if (c == '\n') {
				line += 1;
				pos += 1;
				continue;
			}
			if (is_separator(c)) {
				pos += 1;
				continue;
			}

			if (c == '/' && pos + 1 < source.size() && source[pos + 1] == '/') {
				while (pos < source.size() && source[pos] != '\n') {
					pos += 1;
				}
				continue;
			}

			if (c == '"') {
				const uint32_t start_line = line;
				const size_t start = pos;
				pos += 1;
				// Strings have no escape sequences, a backslash is literal and only the closing quote ends the string.
				while (pos < source.size() && source[pos] != '"') {
					if (source[pos] == '\n') {
						line += 1;
					}
					pos += 1;
				}
				if (pos >= source.size()) {
					return failure(std::format("Unterminated string starting at line {}", start_line));
				}
				pos += 1; // closing quote
				tokens.push_back({source.substr(start, pos - start), start_line});
				continue;
			}

			if (is_punct(c)) {
				tokens.push_back({source.substr(pos, 1), line});
				pos += 1;
				continue;
			}

			const size_t start = pos;
			while (pos < source.size()) {
				const char ch = source[pos];
				if (is_separator(ch) || is_punct(ch) || ch == '"') {
					break;
				}
				if (ch == '/' && pos + 1 < source.size() && (source[pos + 1] == '/' || source[pos + 1] == '*')) {
					break;
				}
				pos += 1;
			}
			tokens.push_back({source.substr(start, pos - start), line});
		}

		return tokens;
	}

	struct MDLReader {
		std::vector<Token> tokens;
		size_t position = 0;

		bool eof() const {
			return position >= tokens.size();
		}

		const Token& peek(const size_t offset = 0) const {
			return tokens[position + offset];
		}

		bool peek_is(const std::string_view t) const {
			return position < tokens.size() && tokens[position].text == t;
		}

		result<Token, std::string> consume() {
			if (position >= tokens.size()) {
				return failure("Unexpected end of file");
			}
			return tokens[position++];
		}

		std::optional<std::string> consume(std::string_view expected) {
			if (position >= tokens.size()) {
				return std::format("Expected '{}', got end of file", expected);
			}
			if (tokens[position].text != expected) {
				return std::format("Expected '{}' at line {}, got '{}'", expected, tokens[position].line, tokens[position].text);
			}
			position += 1;
			return std::nullopt;
		}

		result<int64_t, std::string> consume_i64() {
			if (position >= tokens.size()) {
				return failure("Expected integer, got end of file");
			}
			const auto& tok = tokens[position];
			int64_t value = 0;
			const auto [ptr, ec] = std::from_chars(tok.text.data(), tok.text.data() + tok.text.size(), value);
			if (ec != std::errc()) {
				return failure(std::format("Expected integer at line {}, got '{}'", tok.line, tok.text));
			}
			position += 1;
			return value;
		}

		result<uint32_t, std::string> consume_u32() {
			OUTCOME_TRY(const auto v, consume_i64());
			return static_cast<uint32_t>(v);
		}

		result<float, std::string> consume_f32() {
			if (position >= tokens.size()) {
				return failure("Expected float, got end of file");
			}
			const auto& tok = tokens[position];
			float value = 0.f;
			const auto [ptr, ec] = std::from_chars(tok.text.data(), tok.text.data() + tok.text.size(), value);
			if (ec != std::errc()) {
				return failure(std::format("Expected number at line {}, got '{}'", tok.line, tok.text));
			}
			position += 1;
			return value;
		}

		result<std::string_view, std::string> consume_quoted_string() {
			if (position >= tokens.size()) {
				return failure(std::string("Expected quoted string, got end of file"));
			}
			const auto& tok = tokens[position];
			if (tok.text.size() < 2 || tok.text.front() != '"' || tok.text.back() != '"') {
				return failure(std::format("Expected quoted string at line {}, got '{}'", tok.line, tok.text));
			}
			position += 1;
			return outcome::success(tok.text.substr(1, tok.text.size() - 2));
		}

		result<glm::vec2, std::string> consume_vec2() {
			TRY(consume("{"));
			glm::vec2 v;
			OUTCOME_TRY(v.x, consume_f32());
			OUTCOME_TRY(v.y, consume_f32());
			TRY(consume("}"));
			return v;
		}

		result<glm::vec3, std::string> consume_vec3() {
			TRY(consume("{"));
			glm::vec3 v;
			OUTCOME_TRY(v.x, consume_f32());
			OUTCOME_TRY(v.y, consume_f32());
			OUTCOME_TRY(v.z, consume_f32());
			TRY(consume("}"));
			return v;
		}

		result<glm::vec4, std::string> consume_vec4() {
			TRY(consume("{"));
			glm::vec4 v;
			OUTCOME_TRY(v.x, consume_f32());
			OUTCOME_TRY(v.y, consume_f32());
			OUTCOME_TRY(v.z, consume_f32());
			OUTCOME_TRY(v.w, consume_f32());
			TRY(consume("}"));
			return v;
		}

		result<glm::quat, std::string> consume_quat() {
			TRY(consume("{"));
			glm::quat q;
			OUTCOME_TRY(q.x, consume_f32());
			OUTCOME_TRY(q.y, consume_f32());
			OUTCOME_TRY(q.z, consume_f32());
			OUTCOME_TRY(q.w, consume_f32());
			TRY(consume("}"));
			return q;
		}

		template<typename T>
		result<T, std::string> consume_value() {
			if constexpr (std::is_same_v<T, float>) {
				return consume_f32();
			} else if constexpr (std::is_same_v<T, uint32_t>) {
				return consume_u32();
			} else if constexpr (std::is_same_v<T, glm::vec2>) {
				return consume_vec2();
			} else if constexpr (std::is_same_v<T, glm::vec3>) {
				return consume_vec3();
			} else if constexpr (std::is_same_v<T, glm::vec4>) {
				return consume_vec4();
			} else if constexpr (std::is_same_v<T, glm::quat>) {
				return consume_quat();
			} else {
				static_assert(sizeof(T) == 0, "Unsupported track value type");
			}
		}

		template<typename T>
		result<TrackHeader<T>, std::string> parse_animated_track(int& unique_tracks) {
			TrackHeader<T> track;
			track.id = unique_tracks++;

			OUTCOME_TRY(auto count, consume_u32());
			TRY(consume("{"));

			OUTCOME_TRY(auto interp_tok, consume());
			if (interp_tok.text == "DontInterp") {
				track.interpolation_type = InterpolationType::none;
			} else if (interp_tok.text == "Linear") {
				track.interpolation_type = InterpolationType::linear;
			} else if (interp_tok.text == "Hermite") {
				track.interpolation_type = InterpolationType::hermite;
			} else if (interp_tok.text == "Bezier") {
				track.interpolation_type = InterpolationType::bezier;
			} else {
				return failure(std::format("Expected interpolation keyword at line {}, got '{}'", interp_tok.line, interp_tok.text));
			}

			if (peek_is("GlobalSeqId")) {
				TRY(consume("GlobalSeqId"));
				OUTCOME_TRY(auto gs, consume_i64());
				track.global_sequence_ID = static_cast<int32_t>(gs);
			}

			track.tracks.reserve(count);
			for (uint32_t i = 0; i < count; i++) {
				Track<T> k {};
				OUTCOME_TRY(auto frame, consume_i64());
				k.frame = static_cast<int32_t>(frame);
				TRY(consume(":"));
				OUTCOME_TRY(k.value, consume_value<T>());

				if (track.interpolation_type == InterpolationType::hermite || track.interpolation_type == InterpolationType::bezier) {
					TRY(consume("InTan"));
					OUTCOME_TRY(k.inTan, consume_value<T>());
					TRY(consume("OutTan"));
					OUTCOME_TRY(k.outTan, consume_value<T>());
				}
				track.tracks.push_back(k);
			}

			TRY(consume("}"));
			return track;
		}

		/// Returns true if the token at the cursor was consumed as a node-level field.
		/// `is_static` tells us whether the dispatcher already consumed a `static` prefix
		/// (no node fields use `static`, so it would be wrong to dispatch to one).
		result<bool, std::string> try_parse_node_field(Node& node, int& unique_tracks, bool is_static) {
			if (is_static || position >= tokens.size()) {
				return false;
			}
			const auto& kw = tokens[position].text;

			if (kw == "ObjectId") {
				position += 1;
				OUTCOME_TRY(auto v, consume_i64());
				node.id = static_cast<int>(v);
				return true;
			}
			if (kw == "Parent") {
				position += 1;
				OUTCOME_TRY(auto v, consume_i64());
				node.parent_id = static_cast<int>(v);
				return true;
			}
			if (kw == "DontInherit") {
				position += 1;
				TRY(consume("{"));
				while (!peek_is("}")) {
					OUTCOME_TRY(auto t, consume());
					if (t.text == "Translation") {
						node.flags |= Node::Flags::dont_inherit_translation;
					} else if (t.text == "Rotation") {
						node.flags |= Node::Flags::dont_inherit_rotation;
					} else if (t.text == "Scaling") {
						node.flags |= Node::Flags::dont_inherit_scaling;
					} else {
						return failure(std::format("Unknown DontInherit member '{}' at line {}", t.text, t.line));
					}
				}
				TRY(consume("}"));
				return true;
			}
			if (kw == "Billboarded") {
				position += 1;
				node.flags |= Node::Flags::billboarded;
				return true;
			}
			if (kw == "BillboardedLockX") {
				position += 1;
				node.flags |= Node::Flags::billboarded_lock_x;
				return true;
			}
			if (kw == "BillboardedLockY") {
				position += 1;
				node.flags |= Node::Flags::billboarded_lock_y;
				return true;
			}
			if (kw == "BillboardedLockZ") {
				position += 1;
				node.flags |= Node::Flags::billboarded_lock_z;
				return true;
			}
			if (kw == "CameraAnchored") {
				position += 1;
				node.flags |= Node::Flags::camera_anchored;
				return true;
			}
			if (kw == "Unshaded") {
				position += 1;
				node.flags |= Node::Flags::unshaded;
				return true;
			}
			if (kw == "SortPrimsFarZ") {
				position += 1;
				node.flags |= Node::Flags::sort_primitives_far_z;
				return true;
			}
			if (kw == "LineEmitter") {
				position += 1;
				node.flags |= Node::Flags::line_emitter;
				return true;
			}
			if (kw == "Unfogged") {
				position += 1;
				node.flags |= Node::Flags::unfogged;
				return true;
			}
			if (kw == "ModelSpace") {
				position += 1;
				node.flags |= Node::Flags::model_space;
				return true;
			}
			if (kw == "XYQuad") {
				position += 1;
				node.flags |= Node::Flags::xy_quad;
				return true;
			}

			if (kw == "Translation") {
				position += 1;
				OUTCOME_TRY(node.KGTR, parse_animated_track<glm::vec3>(unique_tracks));
				return true;
			}
			if (kw == "Rotation") {
				position += 1;
				OUTCOME_TRY(node.KGRT, parse_animated_track<glm::quat>(unique_tracks));
				return true;
			}
			if (kw == "Scaling") {
				position += 1;
				OUTCOME_TRY(node.KGSC, parse_animated_track<glm::vec3>(unique_tracks));
				return true;
			}

			return false;
		}
	};

	static outcome::result<void, std::string> parse_version(MDLReader& r, MDX& mdx) {
		TRY(r.consume("Version"));
		TRY(r.consume("{"));
		TRY(r.consume("FormatVersion"));
		OUTCOME_TRY(auto v, r.consume_i64());
		mdx.version = static_cast<uint32_t>(v);
		if (mdx.version != 800 && mdx.version != 900 && mdx.version != 1000 && mdx.version != 1100 && mdx.version != 1200) {
			return failure(std::format("Unsupported FormatVersion {}", mdx.version));
		}
		TRY(r.consume("}"));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_model(MDLReader& r, MDX& mdx) {
		TRY(r.consume("Model"));
		OUTCOME_TRY(mdx.name, r.consume_quoted_string());
		TRY(r.consume("{"));
		while (!r.peek_is("}")) {
			OUTCOME_TRY(auto kw, r.consume());
			if (kw.text == "BlendTime") {
				OUTCOME_TRY(mdx.blend_time, r.consume_u32());
			} else if (kw.text == "MinimumExtent") {
				OUTCOME_TRY(mdx.extent.minimum, r.consume_vec3());
			} else if (kw.text == "MaximumExtent") {
				OUTCOME_TRY(mdx.extent.maximum, r.consume_vec3());
			} else if (kw.text == "BoundsRadius") {
				OUTCOME_TRY(mdx.extent.bounds_radius, r.consume_f32());
			} else if (kw.text == "NumGeosets" || kw.text == "NumGeosetAnims" || kw.text == "NumHelpers" || kw.text == "NumLights"
					   || kw.text == "NumBones" || kw.text == "NumAttachments" || kw.text == "NumParticleEmitters"
					   || kw.text == "NumParticleEmitters2" || kw.text == "NumRibbonEmitters" || kw.text == "NumEvents"
					   || kw.text == "NumFaceFX") {
				// Informational counts only so we can discard, the actual lists are authoritative.
				OUTCOME_TRY(r.consume_i64());
			} else {
				return failure(std::format("Model: unknown field '{}' at line {}", kw.text, kw.line));
			}
		}
		TRY(r.consume("}"));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_sequences(MDLReader& r, MDX& mdx) {
		TRY(r.consume("Sequences"));
		OUTCOME_TRY(auto count, r.consume_u32());
		TRY(r.consume("{"));
		for (uint32_t i = 0; i < count; i++) {
			TRY(r.consume("Anim"));
			Sequence seq {};
			OUTCOME_TRY(seq.name, r.consume_quoted_string());
			TRY(r.consume("{"));
			while (!r.peek_is("}")) {
				OUTCOME_TRY(auto kw, r.consume());
				if (kw.text == "Interval") {
					TRY(r.consume("{"));
					OUTCOME_TRY(seq.start_frame, r.consume_u32());
					OUTCOME_TRY(seq.end_frame, r.consume_u32());
					TRY(r.consume("}"));
				} else if (kw.text == "MoveSpeed") {
					OUTCOME_TRY(seq.movespeed, r.consume_f32());
				} else if (kw.text == "NonLooping") {
					seq.flags |= Sequence::Flags::non_looping;
				} else if (kw.text == "Rarity") {
					OUTCOME_TRY(seq.rarity, r.consume_f32());
				} else if (kw.text == "SyncPoint") {
					OUTCOME_TRY(seq.sync_point, r.consume_u32());
				} else if (kw.text == "MinimumExtent") {
					OUTCOME_TRY(seq.extent.minimum, r.consume_vec3());
				} else if (kw.text == "MaximumExtent") {
					OUTCOME_TRY(seq.extent.maximum, r.consume_vec3());
				} else if (kw.text == "BoundsRadius") {
					OUTCOME_TRY(seq.extent.bounds_radius, r.consume_f32());
				} else {
					return failure(std::format("Anim: unknown field '{}' at line {}", kw.text, kw.line));
				}
			}
			TRY(r.consume("}"));
			mdx.sequences.push_back(std::move(seq));
		}
		TRY(r.consume("}"));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_global_sequences(MDLReader& r, MDX& mdx) {
		TRY(r.consume("GlobalSequences"));
		OUTCOME_TRY(auto count, r.consume_u32());
		TRY(r.consume("{"));
		for (uint32_t i = 0; i < count; i++) {
			TRY(r.consume("Duration"));
			OUTCOME_TRY(auto d, r.consume_u32());
			mdx.global_sequences.push_back(d);
		}
		TRY(r.consume("}"));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_textures(MDLReader& r, MDX& mdx) {
		TRY(r.consume("Textures"));
		OUTCOME_TRY(auto count, r.consume_u32());
		TRY(r.consume("{"));
		for (uint32_t i = 0; i < count; i++) {
			TRY(r.consume("Bitmap"));
			TRY(r.consume("{"));
			Texture tex {};
			while (!r.peek_is("}")) {
				OUTCOME_TRY(auto kw, r.consume());
				if (kw.text == "Image") {
					OUTCOME_TRY(auto path, r.consume_quoted_string());
					tex.file_name = path;
				} else if (kw.text == "ReplaceableId") {
					OUTCOME_TRY(tex.replaceable_id, r.consume_u32());
				} else if (kw.text == "WrapWidth") {
					tex.flags |= Texture::Flags::wrap_width;
				} else if (kw.text == "WrapHeight") {
					tex.flags |= Texture::Flags::wrap_height;
				} else {
					return failure(std::format("Bitmap: unknown field '{}' at line {}", kw.text, kw.line));
				}
			}
			TRY(r.consume("}"));
			mdx.textures.push_back(std::move(tex));
		}
		TRY(r.consume("}"));
		return outcome::success();
	}

	static uint32_t parse_material_filter_mode(const std::string_view s) {
		if (s == "None") {
			return 0;
		}
		if (s == "Transparent") {
			return 1;
		}
		if (s == "Blend") {
			return 2;
		}
		if (s == "Additive") {
			return 3;
		}
		if (s == "AddAlpha") {
			return 4;
		}
		if (s == "Modulate") {
			return 5;
		}
		if (s == "Modulate2x") {
			return 6;
		}
		return 0xFFFFFFFFu;
	}

	static outcome::result<void, std::string> parse_layer(MDLReader& r, Material& material, int& unique_tracks, bool is_hd) {
		TRY(r.consume("Layer"));
		TRY(r.consume("{"));
		Layer layer {};
		layer.shader = is_hd ? ShaderType::HD : ShaderType::SD;
		layer.texture_animation_id = 0xFFFFFFFFu;
		layer.alpha = 1.f;
		layer.emissive_gain = 1.f;
		layer.fresnel_color = glm::vec3(1.f);

		while (!r.peek_is("}")) {
			bool is_static = false;
			if (r.peek_is("static")) {
				TRY(r.consume("static"));
				is_static = true;
			}
			OUTCOME_TRY(auto kw, r.consume());

			if (kw.text == "Shader") {
				// Per-layer HD shader name (v1100+). Material-level Shader is handled by the caller.
				OUTCOME_TRY(auto shader_name, r.consume_quoted_string());
				if (!shader_name.empty()) {
					layer.shader = ShaderType::HD;
				}
			} else if (kw.text == "FilterMode") {
				OUTCOME_TRY(auto mode_tok, r.consume());
				const uint32_t fm = parse_material_filter_mode(mode_tok.text);
				if (fm == 0xFFFFFFFFu) {
					return failure(std::format("Unknown FilterMode '{}' at line {}", mode_tok.text, mode_tok.line));
				}
				layer.blend_mode = fm;
			} else if (kw.text == "Unshaded") {
				layer.shading_flags |= Layer::ShadingFlags::unshaded;
			} else if (kw.text == "SphereEnvMap") {
				layer.shading_flags |= Layer::ShadingFlags::sphere_environment_map;
			} else if (kw.text == "TwoSided") {
				layer.shading_flags |= Layer::ShadingFlags::two_sided;
			} else if (kw.text == "Unfogged") {
				layer.shading_flags |= Layer::ShadingFlags::unfogged;
			} else if (kw.text == "NoDepthTest") {
				layer.shading_flags |= Layer::ShadingFlags::no_depth_test;
			} else if (kw.text == "NoDepthSet") {
				layer.shading_flags |= Layer::ShadingFlags::no_depth_set;
			} else if (kw.text == "TVertexAnimId") {
				OUTCOME_TRY(layer.texture_animation_id, r.consume_u32());
			} else if (kw.text == "CoordId") {
				OUTCOME_TRY(layer.coord_id, r.consume_u32());
			} else if (kw.text == "TextureID") {
				LayerTexture texture {};
				if (is_static) {
					OUTCOME_TRY(texture.id, r.consume_u32());
					if (r.peek_is("<=")) {
						TRY(r.consume("<="));
						OUTCOME_TRY(texture.slot, r.consume_u32());
					}
				} else {
					OUTCOME_TRY(texture.KMTF, r.parse_animated_track<uint32_t>(unique_tracks));
				}
				layer.textures.push_back(std::move(texture));
			} else if (kw.text == "Alpha") {
				if (is_static) {
					OUTCOME_TRY(layer.alpha, r.consume_f32());
				} else {
					OUTCOME_TRY(layer.KMTA, r.parse_animated_track<float>(unique_tracks));
				}
			} else if (kw.text == "EmissiveGain") {
				if (is_static) {
					OUTCOME_TRY(layer.emissive_gain, r.consume_f32());
				} else {
					OUTCOME_TRY(layer.KMTE, r.parse_animated_track<float>(unique_tracks));
				}
			} else if (kw.text == "FresnelColor") {
				if (is_static) {
					OUTCOME_TRY(layer.fresnel_color, r.consume_vec3());
				} else {
					OUTCOME_TRY(layer.KFC3, r.parse_animated_track<glm::vec3>(unique_tracks));
				}
			} else if (kw.text == "FresnelOpacity") {
				if (is_static) {
					OUTCOME_TRY(layer.fresnel_opacity, r.consume_f32());
				} else {
					OUTCOME_TRY(layer.KFCA, r.parse_animated_track<float>(unique_tracks));
				}
			} else if (kw.text == "FresnelTeamColor") {
				if (is_static) {
					OUTCOME_TRY(layer.fresnel_team_color, r.consume_f32());
				} else {
					OUTCOME_TRY(layer.KFTC, r.parse_animated_track<float>(unique_tracks));
				}
			} else {
				return failure(std::format("Layer: unknown field '{}' at line {}", kw.text, kw.line));
			}
		}
		TRY(r.consume("}"));
		if (layer.textures.empty()) {
			layer.textures.push_back(LayerTexture {});
		}
		material.layers.push_back(std::move(layer));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_materials(MDLReader& r, MDX& mdx) {
		TRY(r.consume("Materials"));
		OUTCOME_TRY(auto count, r.consume_u32());
		TRY(r.consume("{"));
		for (uint32_t i = 0; i < count; i++) {
			TRY(r.consume("Material"));
			TRY(r.consume("{"));
			Material material {};
			bool is_hd = false;
			while (!r.peek_is("}")) {
				if (r.peek_is("Shader")) {
					TRY(r.consume("Shader"));
					OUTCOME_TRY(auto shader, r.consume_quoted_string());
					is_hd = !shader.empty();
				} else if (r.peek_is("PriorityPlane")) {
					TRY(r.consume("PriorityPlane"));
					OUTCOME_TRY(material.priority_plane, r.consume_u32());
				} else if (r.peek_is("ConstantColor")) {
					TRY(r.consume("ConstantColor"));
					material.flags |= Material::Flags::constant_color;
				} else if (r.peek_is("SortPrimsNearZ")) {
					TRY(r.consume("SortPrimsNearZ"));
					material.flags |= Material::Flags::sort_primitives_near_z;
				} else if (r.peek_is("SortPrimsFarZ")) {
					TRY(r.consume("SortPrimsFarZ"));
					material.flags |= Material::Flags::sort_primitives_far_z;
				} else if (r.peek_is("FullResolution")) {
					TRY(r.consume("FullResolution"));
					material.flags |= Material::Flags::full_resolution;
				} else if (r.peek_is("Layer")) {
					OUTCOME_TRY(parse_layer(r, material, mdx.unique_tracks, is_hd));
				} else {
					const auto& tok = r.peek();
					return failure(std::format("Material: unknown field '{}' at line {}", tok.text, tok.line));
				}
			}
			TRY(r.consume("}"));
			mdx.materials.push_back(std::move(material));
		}
		TRY(r.consume("}"));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_texture_anims(MDLReader& r, MDX& mdx) {
		TRY(r.consume("TextureAnims"));
		OUTCOME_TRY(auto count, r.consume_u32());
		TRY(r.consume("{"));
		for (uint32_t i = 0; i < count; i++) {
			TRY(r.consume("TVertexAnim"));
			TRY(r.consume("{"));
			TextureAnimation ta {};
			while (!r.peek_is("}")) {
				OUTCOME_TRY(auto kw, r.consume());
				if (kw.text == "Translation") {
					OUTCOME_TRY(ta.KTAT, r.parse_animated_track<glm::vec3>(mdx.unique_tracks));
				} else if (kw.text == "Rotation") {
					OUTCOME_TRY(ta.KTAR, r.parse_animated_track<glm::quat>(mdx.unique_tracks));
				} else if (kw.text == "Scaling") {
					OUTCOME_TRY(ta.KTAS, r.parse_animated_track<glm::vec3>(mdx.unique_tracks));
				} else {
					return failure(std::format("TVertexAnim: unknown field '{}' at line {}", kw.text, kw.line));
				}
			}
			TRY(r.consume("}"));
			mdx.texture_animations.push_back(std::move(ta));
		}
		TRY(r.consume("}"));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_geoset(MDLReader& r, MDX& mdx) {
		TRY(r.consume("Geoset"));
		TRY(r.consume("{"));
		Geoset g {};

		while (!r.peek_is("}")) {
			OUTCOME_TRY(auto kw, r.consume());
			if (kw.text == "Vertices") {
				OUTCOME_TRY(auto n, r.consume_u32());
				TRY(r.consume("{"));
				g.vertices.reserve(n);
				for (uint32_t i = 0; i < n; i++) {
					OUTCOME_TRY(auto v, r.consume_vec3());
					g.vertices.push_back(v);
				}
				TRY(r.consume("}"));
			} else if (kw.text == "Normals") {
				OUTCOME_TRY(auto n, r.consume_u32());
				TRY(r.consume("{"));
				g.normals.reserve(n);
				for (uint32_t i = 0; i < n; i++) {
					OUTCOME_TRY(auto v, r.consume_vec3());
					g.normals.push_back(v);
				}
				TRY(r.consume("}"));
			} else if (kw.text == "TVertices") {
				OUTCOME_TRY(auto n, r.consume_u32());
				TRY(r.consume("{"));
				std::vector<glm::vec2> uvs;
				uvs.reserve(n);
				for (uint32_t i = 0; i < n; i++) {
					OUTCOME_TRY(auto v, r.consume_vec2());
					uvs.push_back(v);
				}
				TRY(r.consume("}"));
				g.uv_sets.push_back(std::move(uvs));
			} else if (kw.text == "Tangents") {
				OUTCOME_TRY(auto n, r.consume_u32());
				TRY(r.consume("{"));
				g.tangents.reserve(n);
				for (uint32_t i = 0; i < n; i++) {
					OUTCOME_TRY(auto v, r.consume_vec4());
					g.tangents.push_back(v);
				}
				TRY(r.consume("}"));
			} else if (kw.text == "SkinWeights") {
				OUTCOME_TRY(auto n, r.consume_u32());
				TRY(r.consume("{"));
				g.skin.reserve(n * 8);
				for (uint32_t i = 0; i < n * 8; i++) {
					OUTCOME_TRY(auto v, r.consume_u32());
					g.skin.push_back(static_cast<uint8_t>(v));
				}
				TRY(r.consume("}"));
			} else if (kw.text == "VertexGroup") {
				TRY(r.consume("{"));
				while (!r.peek_is("}")) {
					OUTCOME_TRY(auto v, r.consume_u32());
					g.vertex_groups.push_back(static_cast<uint8_t>(v));
				}
				TRY(r.consume("}"));
			} else if (kw.text == "Faces") {
				OUTCOME_TRY(auto face_groups, r.consume_u32());
				OUTCOME_TRY(auto index_count, r.consume_u32());
				(void)face_groups;
				TRY(r.consume("{"));
				TRY(r.consume("Triangles"));
				TRY(r.consume("{"));
				g.faces.reserve(index_count);
				// Accept either one combined `{ i, i, i, ... }` block OR many per-triangle `{ a, b, c }` blocks.
				while (!r.peek_is("}")) {
					TRY(r.consume("{"));
					while (!r.peek_is("}")) {
						OUTCOME_TRY(auto v, r.consume_u32());
						g.faces.push_back(static_cast<uint16_t>(v));
					}
					TRY(r.consume("}"));
				}
				TRY(r.consume("}"));
				TRY(r.consume("}"));
			} else if (kw.text == "Groups") {
				OUTCOME_TRY(auto group_count, r.consume_u32());
				OUTCOME_TRY(auto total, r.consume_u32());
				(void)total;
				TRY(r.consume("{"));
				for (uint32_t i = 0; i < group_count; i++) {
					TRY(r.consume("Matrices"));
					TRY(r.consume("{"));
					uint32_t size_in_group = 0;
					while (!r.peek_is("}")) {
						OUTCOME_TRY(auto idx, r.consume_u32());
						g.matrix_indices.push_back(idx);
						size_in_group += 1;
					}
					TRY(r.consume("}"));
					g.matrix_groups.push_back(size_in_group);
				}
				TRY(r.consume("}"));
			} else if (kw.text == "MinimumExtent") {
				OUTCOME_TRY(g.extent.minimum, r.consume_vec3());
			} else if (kw.text == "MaximumExtent") {
				OUTCOME_TRY(g.extent.maximum, r.consume_vec3());
			} else if (kw.text == "BoundsRadius") {
				OUTCOME_TRY(g.extent.bounds_radius, r.consume_f32());
			} else if (kw.text == "Anim") {
				TRY(r.consume("{"));
				Extent e {};
				while (!r.peek_is("}")) {
					OUTCOME_TRY(auto akw, r.consume());
					if (akw.text == "MinimumExtent") {
						OUTCOME_TRY(e.minimum, r.consume_vec3());
					} else if (akw.text == "MaximumExtent") {
						OUTCOME_TRY(e.maximum, r.consume_vec3());
					} else if (akw.text == "BoundsRadius") {
						OUTCOME_TRY(e.bounds_radius, r.consume_f32());
					} else {
						return failure(std::format("Geoset Anim: unknown field '{}' at line {}", akw.text, akw.line));
					}
				}
				TRY(r.consume("}"));
				g.sequence_extents.push_back(e);
			} else if (kw.text == "MaterialID") {
				OUTCOME_TRY(g.material_id, r.consume_u32());
			} else if (kw.text == "SelectionGroup") {
				OUTCOME_TRY(g.selection_group, r.consume_u32());
			} else if (kw.text == "Unselectable") {
				g.selection_flags |= 4;
			} else if (kw.text == "LevelOfDetail") {
				OUTCOME_TRY(g.lod, r.consume_u32());
			} else if (kw.text == "Name") {
				OUTCOME_TRY(g.lod_name, r.consume_quoted_string());
			} else {
				return failure(std::format("Geoset: unknown field '{}' at line {}", kw.text, kw.line));
			}
		}
		TRY(r.consume("}"));
		mdx.geosets.push_back(std::move(g));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_geoset_anim(MDLReader& r, MDX& mdx) {
		TRY(r.consume("GeosetAnim"));
		TRY(r.consume("{"));
		GeosetAnimation ga {};
		ga.alpha = 1.f;
		ga.color = glm::vec3(1.f);
		ga.geoset_id = 0xFFFFFFFFu;

		while (!r.peek_is("}")) {
			bool is_static = false;
			if (r.peek_is("static")) {
				TRY(r.consume("static"));
				is_static = true;
			}
			OUTCOME_TRY(auto kw, r.consume());
			if (kw.text == "GeosetId") {
				if (r.peek_is("None")) {
					TRY(r.consume("None"));
					ga.geoset_id = 0xFFFFFFFFu;
				} else {
					OUTCOME_TRY(ga.geoset_id, r.consume_u32());
				}
			} else if (kw.text == "Alpha") {
				if (is_static) {
					OUTCOME_TRY(ga.alpha, r.consume_f32());
				} else {
					OUTCOME_TRY(ga.KGAO, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "Color") {
				if (is_static) {
					OUTCOME_TRY(ga.color, r.consume_vec3());
				} else {
					OUTCOME_TRY(ga.KGAC, r.parse_animated_track<glm::vec3>(mdx.unique_tracks));
				}
			} else if (kw.text == "DropShadow") {
				ga.flags |= 0x1;
			} else {
				return failure(std::format("GeosetAnim: unknown field '{}' at line {}", kw.text, kw.line));
			}
		}
		TRY(r.consume("}"));
		mdx.animations.push_back(std::move(ga));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_bone(MDLReader& r, MDX& mdx) {
		TRY(r.consume("Bone"));
		Bone b {};
		b.geoset_id = -1;
		b.geoset_animation_id = -1;
		b.node.id = -1;
		b.node.parent_id = -1;
		b.node.flags = 0x100; // bone
		OUTCOME_TRY(b.node.name, r.consume_quoted_string());
		TRY(r.consume("{"));

		while (!r.peek_is("}")) {
			OUTCOME_TRY(auto handled, r.try_parse_node_field(b.node, mdx.unique_tracks, false));
			if (handled) {
				continue;
			}

			OUTCOME_TRY(auto kw, r.consume());
			if (kw.text == "GeosetId") {
				if (r.peek_is("Multiple")) {
					TRY(r.consume("Multiple"));
					b.geoset_id = -2;
				} else if (r.peek_is("None")) {
					TRY(r.consume("None"));
					b.geoset_id = -1;
				} else {
					OUTCOME_TRY(auto v, r.consume_i64());
					b.geoset_id = static_cast<int32_t>(v);
				}
			} else if (kw.text == "GeosetAnimId") {
				if (r.peek_is("None")) {
					TRY(r.consume("None"));
					b.geoset_animation_id = -1;
				} else {
					OUTCOME_TRY(auto v, r.consume_i64());
					b.geoset_animation_id = static_cast<int32_t>(v);
				}
			} else {
				return failure(std::format("Bone: unknown field '{}' at line {}", kw.text, kw.line));
			}
		}
		TRY(r.consume("}"));
		mdx.bones.push_back(std::move(b));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_helper(MDLReader& r, MDX& mdx) {
		TRY(r.consume("Helper"));
		Node n {};
		n.id = -1;
		n.parent_id = -1;
		n.flags = 0;
		OUTCOME_TRY(n.name, r.consume_quoted_string());
		TRY(r.consume("{"));
		while (!r.peek_is("}")) {
			OUTCOME_TRY(auto handled, r.try_parse_node_field(n, mdx.unique_tracks, false));
			if (handled) {
				continue;
			}
			const auto& tok = r.peek();
			return failure(std::format("Helper: unknown field '{}' at line {}", tok.text, tok.line));
		}
		TRY(r.consume("}"));
		mdx.help_bones.push_back(std::move(n));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_light(MDLReader& r, MDX& mdx) {
		TRY(r.consume("Light"));
		Light light {};
		light.node.id = -1;
		light.node.parent_id = -1;
		light.node.flags = Node::Flags::light;
		light.shadow_intensity = 0.4f;
		OUTCOME_TRY(light.node.name, r.consume_quoted_string());
		TRY(r.consume("{"));

		while (!r.peek_is("}")) {
			OUTCOME_TRY(auto handled, r.try_parse_node_field(light.node, mdx.unique_tracks, false));
			if (handled) {
				continue;
			}

			bool is_static = false;
			if (r.peek_is("static")) {
				TRY(r.consume("static"));
				is_static = true;
			}
			OUTCOME_TRY(auto kw, r.consume());
			if (kw.text == "Omnidirectional") {
				light.type = 0;
			} else if (kw.text == "Directional") {
				light.type = 1;
			} else if (kw.text == "Ambient") {
				light.type = 2;
			} else if (kw.text == "AttenuationStart") {
				if (is_static) {
					OUTCOME_TRY(auto v, r.consume_u32());
					light.attenuation_start = static_cast<float>(v);
				} else {
					OUTCOME_TRY(light.KLAS, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "AttenuationEnd") {
				if (is_static) {
					OUTCOME_TRY(auto v, r.consume_u32());
					light.attenuation_end = static_cast<float>(v);
				} else {
					OUTCOME_TRY(light.KLAE, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "Intensity") {
				if (is_static) {
					OUTCOME_TRY(light.intensity, r.consume_f32());
				} else {
					OUTCOME_TRY(light.KLAI, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "Color") {
				if (is_static) {
					OUTCOME_TRY(light.color, r.consume_vec3());
				} else {
					OUTCOME_TRY(light.KLAC, r.parse_animated_track<glm::vec3>(mdx.unique_tracks));
				}
			} else if (kw.text == "AmbIntensity") {
				if (is_static) {
					OUTCOME_TRY(light.ambient_intensity, r.consume_f32());
				} else {
					OUTCOME_TRY(light.KLBI, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "AmbColor") {
				if (is_static) {
					OUTCOME_TRY(light.ambient_color, r.consume_vec3());
				} else {
					OUTCOME_TRY(light.KLBC, r.parse_animated_track<glm::vec3>(mdx.unique_tracks));
				}
			} else if (kw.text == "Visibility") {
				OUTCOME_TRY(light.KLAV, r.parse_animated_track<float>(mdx.unique_tracks));
			} else {
				return failure(std::format("Light: unknown field '{}' at line {}", kw.text, kw.line));
			}
		}
		TRY(r.consume("}"));
		mdx.lights.push_back(std::move(light));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_attachment(MDLReader& r, MDX& mdx) {
		TRY(r.consume("Attachment"));
		Attachment att {};
		att.node.id = -1;
		att.node.parent_id = -1;
		att.node.flags = Node::Flags::attachment;
		att.attachment_id = 0;
		att.reserved = 0;
		OUTCOME_TRY(att.node.name, r.consume_quoted_string());
		TRY(r.consume("{"));
		while (!r.peek_is("}")) {
			OUTCOME_TRY(auto handled, r.try_parse_node_field(att.node, mdx.unique_tracks, false));
			if (handled) {
				continue;
			}
			OUTCOME_TRY(auto kw, r.consume());
			if (kw.text == "AttachmentID") {
				OUTCOME_TRY(auto v, r.consume_i64());
				att.attachment_id = static_cast<int>(v);
			} else if (kw.text == "Path") {
				OUTCOME_TRY(att.path, r.consume_quoted_string());
			} else if (kw.text == "Visibility") {
				OUTCOME_TRY(att.KATV, r.parse_animated_track<float>(mdx.unique_tracks));
			} else {
				return failure(std::format("Attachment: unknown field '{}' at line {}", kw.text, kw.line));
			}
		}
		TRY(r.consume("}"));
		mdx.attachments.push_back(std::move(att));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_pivots(MDLReader& r, MDX& mdx) {
		TRY(r.consume("PivotPoints"));
		OUTCOME_TRY(auto count, r.consume_u32());
		TRY(r.consume("{"));
		for (uint32_t i = 0; i < count; i++) {
			OUTCOME_TRY(auto v, r.consume_vec3());
			mdx.pivots.push_back(v);
		}
		TRY(r.consume("}"));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_emitter1(MDLReader& r, MDX& mdx) {
		TRY(r.consume("ParticleEmitter"));
		ParticleEmitter1 e {};
		e.node.id = -1;
		e.node.parent_id = -1;
		e.node.flags = Node::Flags::emitter;
		OUTCOME_TRY(e.node.name, r.consume_quoted_string());
		TRY(r.consume("{"));
		while (!r.peek_is("}")) {
			OUTCOME_TRY(auto handled, r.try_parse_node_field(e.node, mdx.unique_tracks, false));
			if (handled) {
				continue;
			}

			bool is_static = false;
			if (r.peek_is("static")) {
				TRY(r.consume("static"));
				is_static = true;
			}
			OUTCOME_TRY(auto kw, r.consume());
			if (kw.text == "EmitterUsesMdl") {
				e.node.flags |= 0x8000;
			} else if (kw.text == "EmitterUsesTga") {
				e.node.flags |= 0x10000;
			} else if (kw.text == "EmissionRate") {
				if (is_static) {
					OUTCOME_TRY(e.emission_rate, r.consume_f32());
				} else {
					OUTCOME_TRY(e.KPEE, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "Gravity") {
				if (is_static) {
					OUTCOME_TRY(e.gravity, r.consume_f32());
				} else {
					OUTCOME_TRY(e.KPEG, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "Longitude") {
				if (is_static) {
					OUTCOME_TRY(e.longitude, r.consume_f32());
				} else {
					OUTCOME_TRY(e.KPLN, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "Latitude") {
				if (is_static) {
					OUTCOME_TRY(e.latitude, r.consume_f32());
				} else {
					OUTCOME_TRY(e.KPLT, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "Visibility") {
				OUTCOME_TRY(e.KPEV, r.parse_animated_track<float>(mdx.unique_tracks));
			} else if (kw.text == "LifeSpan") {
				if (is_static) {
					OUTCOME_TRY(e.life_span, r.consume_f32());
				} else {
					OUTCOME_TRY(e.KPEL, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "InitVelocity") {
				if (is_static) {
					OUTCOME_TRY(e.speed, r.consume_f32());
				} else {
					OUTCOME_TRY(e.KPES, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "Path") {
				OUTCOME_TRY(e.path, r.consume_quoted_string());
			} else {
				return failure(std::format("ParticleEmitter: unknown field '{}' at line {}", kw.text, kw.line));
			}
		}
		TRY(r.consume("}"));
		mdx.emitters1.push_back(std::move(e));
		return outcome::success();
	}

	static uint32_t parse_emitter2_filter_mode(const std::string_view s) {
		if (s == "Blend") {
			return 0;
		}
		if (s == "Additive") {
			return 1;
		}
		if (s == "Modulate") {
			return 2;
		}
		if (s == "Modulate2x") {
			return 3;
		}
		if (s == "AlphaKey") {
			return 4;
		}
		return 0xFFFFFFFFu;
	}

	static outcome::result<void, std::string> parse_emitter2(MDLReader& r, MDX& mdx) {
		TRY(r.consume("ParticleEmitter2"));
		ParticleEmitter2 e {};
		e.node.id = -1;
		e.node.parent_id = -1;
		e.node.flags = Node::Flags::emitter;
		OUTCOME_TRY(e.node.name, r.consume_quoted_string());
		TRY(r.consume("{"));

		while (!r.peek_is("}")) {
			OUTCOME_TRY(auto handled, r.try_parse_node_field(e.node, mdx.unique_tracks, false));
			if (handled) {
				continue;
			}

			bool is_static = false;
			if (r.peek_is("static")) {
				TRY(r.consume("static"));
				is_static = true;
			}
			OUTCOME_TRY(auto kw, r.consume());
			if (!is_static) {
				const uint32_t fm = parse_emitter2_filter_mode(kw.text);
				if (fm != 0xFFFFFFFFu) {
					e.filter_mode = fm;
					continue;
				}
			}
			if (kw.text == "Squirt") {
				e.squirt = 1;
			} else if (kw.text == "Head") {
				e.head_or_tail = 0;
			} else if (kw.text == "Tail") {
				e.head_or_tail = 1;
			} else if (kw.text == "Both") {
				e.head_or_tail = 2;
			} else if (kw.text == "Speed") {
				if (is_static) {
					OUTCOME_TRY(e.speed, r.consume_f32());
				} else {
					OUTCOME_TRY(e.KP2S, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "Variation") {
				if (is_static) {
					OUTCOME_TRY(e.speed_variation, r.consume_f32());
				} else {
					OUTCOME_TRY(e.KP2R, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "Latitude") {
				if (is_static) {
					OUTCOME_TRY(e.latitude, r.consume_f32());
				} else {
					OUTCOME_TRY(e.KP2L, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "Gravity") {
				if (is_static) {
					OUTCOME_TRY(e.gravity, r.consume_f32());
				} else {
					OUTCOME_TRY(e.KP2G, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "EmissionRate") {
				if (is_static) {
					OUTCOME_TRY(e.emission_rate, r.consume_f32());
				} else {
					OUTCOME_TRY(e.KP2E, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "Width") {
				if (is_static) {
					OUTCOME_TRY(e.width, r.consume_f32());
				} else {
					OUTCOME_TRY(e.KP2W, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "Length") {
				if (is_static) {
					OUTCOME_TRY(e.length, r.consume_f32());
				} else {
					OUTCOME_TRY(e.KP2N, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "Visibility") {
				OUTCOME_TRY(e.KP2V, r.parse_animated_track<float>(mdx.unique_tracks));
			} else if (kw.text == "LifeSpan") {
				OUTCOME_TRY(e.life_span, r.consume_f32());
			} else if (kw.text == "TailLength") {
				OUTCOME_TRY(e.tail_length, r.consume_f32());
			} else if (kw.text == "Time") {
				OUTCOME_TRY(e.time_middle, r.consume_f32());
			} else if (kw.text == "Rows") {
				OUTCOME_TRY(e.rows, r.consume_u32());
			} else if (kw.text == "Columns") {
				OUTCOME_TRY(e.columns, r.consume_u32());
			} else if (kw.text == "TextureID") {
				OUTCOME_TRY(e.texture_id, r.consume_u32());
			} else if (kw.text == "ReplaceableId") {
				OUTCOME_TRY(e.replaceable_id, r.consume_u32());
			} else if (kw.text == "PriorityPlane") {
				OUTCOME_TRY(e.priority_plane, r.consume_u32());
			} else if (kw.text == "Alpha") {
				OUTCOME_TRY(auto v, r.consume_vec3());
				e.segment_alphas = glm::u8vec3(static_cast<uint8_t>(v.x), static_cast<uint8_t>(v.y), static_cast<uint8_t>(v.z));
			} else if (kw.text == "ParticleScaling") {
				OUTCOME_TRY(e.segment_scaling, r.consume_vec3());
			} else if (kw.text == "LifeSpanUVAnim") {
				OUTCOME_TRY(auto v, r.consume_vec3());
				e.head_intervals = glm::uvec3(static_cast<uint32_t>(v.x), static_cast<uint32_t>(v.y), static_cast<uint32_t>(v.z));
			} else if (kw.text == "DecayUVAnim") {
				OUTCOME_TRY(auto v, r.consume_vec3());
				e.head_decay_intervals = glm::uvec3(static_cast<uint32_t>(v.x), static_cast<uint32_t>(v.y), static_cast<uint32_t>(v.z));
			} else if (kw.text == "TailUVAnim") {
				OUTCOME_TRY(auto v, r.consume_vec3());
				e.tail_intervals = glm::uvec3(static_cast<uint32_t>(v.x), static_cast<uint32_t>(v.y), static_cast<uint32_t>(v.z));
			} else if (kw.text == "TailDecayUVAnim") {
				OUTCOME_TRY(auto v, r.consume_vec3());
				e.tail_decay_intervals = glm::uvec3(static_cast<uint32_t>(v.x), static_cast<uint32_t>(v.y), static_cast<uint32_t>(v.z));
			} else if (kw.text == "SegmentColor") {
				TRY(r.consume("{"));
				int color_index = 0;
				while (!r.peek_is("}")) {
					TRY(r.consume("Color"));
					OUTCOME_TRY(auto c, r.consume_vec3());
					if (color_index == 0) {
						e.start_segment_color = c;
					} else if (color_index == 1) {
						e.middle_segment_color = c;
					} else if (color_index == 2) {
						e.end_segment_color = c;
					}
					color_index += 1;
				}
				TRY(r.consume("}"));
			} else {
				return failure(std::format("ParticleEmitter2: unknown field '{}' at line {}", kw.text, kw.line));
			}
		}
		TRY(r.consume("}"));
		mdx.emitters2.push_back(std::move(e));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_ribbon(MDLReader& r, MDX& mdx) {
		TRY(r.consume("RibbonEmitter"));
		RibbonEmitter rb {};
		rb.node.id = -1;
		rb.node.parent_id = -1;
		rb.node.flags = Node::Flags::ribbon_emitter;
		OUTCOME_TRY(rb.node.name, r.consume_quoted_string());
		TRY(r.consume("{"));
		while (!r.peek_is("}")) {
			OUTCOME_TRY(auto handled, r.try_parse_node_field(rb.node, mdx.unique_tracks, false));
			if (handled) {
				continue;
			}

			bool is_static = false;
			if (r.peek_is("static")) {
				TRY(r.consume("static"));
				is_static = true;
			}
			OUTCOME_TRY(auto kw, r.consume());
			if (kw.text == "HeightAbove") {
				if (is_static) {
					OUTCOME_TRY(rb.height_above, r.consume_f32());
				} else {
					OUTCOME_TRY(rb.KRHA, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "HeightBelow") {
				if (is_static) {
					OUTCOME_TRY(rb.height_below, r.consume_f32());
				} else {
					OUTCOME_TRY(rb.KRHB, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "Alpha") {
				if (is_static) {
					OUTCOME_TRY(rb.alpha, r.consume_f32());
				} else {
					OUTCOME_TRY(rb.KRAL, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "Color") {
				if (is_static) {
					OUTCOME_TRY(rb.color, r.consume_vec3());
				} else {
					OUTCOME_TRY(rb.KRCO, r.parse_animated_track<glm::vec3>(mdx.unique_tracks));
				}
			} else if (kw.text == "TextureSlot") {
				if (is_static) {
					OUTCOME_TRY(rb.texture_slot, r.consume_u32());
				} else {
					OUTCOME_TRY(rb.KRTX, r.parse_animated_track<uint32_t>(mdx.unique_tracks));
				}
			} else if (kw.text == "Visibility") {
				if (is_static) {
					OUTCOME_TRY(auto v, r.consume_f32());
					(void)v;
				} else {
					OUTCOME_TRY(rb.KRVS, r.parse_animated_track<float>(mdx.unique_tracks));
				}
			} else if (kw.text == "LifeSpan") {
				OUTCOME_TRY(rb.life_span, r.consume_f32());
			} else if (kw.text == "EmissionRate") {
				OUTCOME_TRY(rb.emission_rate, r.consume_u32());
			} else if (kw.text == "Rows") {
				OUTCOME_TRY(rb.rows, r.consume_u32());
			} else if (kw.text == "Columns") {
				OUTCOME_TRY(rb.columns, r.consume_u32());
			} else if (kw.text == "MaterialID") {
				OUTCOME_TRY(rb.material_id, r.consume_u32());
			} else if (kw.text == "Gravity") {
				OUTCOME_TRY(rb.gravity, r.consume_f32());
			} else {
				return failure(std::format("RibbonEmitter: unknown field '{}' at line {}", kw.text, kw.line));
			}
		}
		TRY(r.consume("}"));
		mdx.ribbons.push_back(std::move(rb));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_event_object(MDLReader& r, MDX& mdx) {
		TRY(r.consume("EventObject"));
		EventObject ev {};
		ev.node.id = -1;
		ev.node.parent_id = -1;
		ev.node.flags = 0;
		ev.global_sequence_id = -1;
		OUTCOME_TRY(ev.node.name, r.consume_quoted_string());
		TRY(r.consume("{"));
		while (!r.peek_is("}")) {
			OUTCOME_TRY(auto handled, r.try_parse_node_field(ev.node, mdx.unique_tracks, false));
			if (handled) {
				continue;
			}
			OUTCOME_TRY(auto kw, r.consume());
			if (kw.text == "EventTrack") {
				OUTCOME_TRY(auto count, r.consume_u32());
				TRY(r.consume("{"));
				if (r.peek_is("GlobalSeqId")) {
					TRY(r.consume("GlobalSeqId"));
					OUTCOME_TRY(auto gs, r.consume_i64());
					ev.global_sequence_id = static_cast<int>(gs);
				}
				ev.times.reserve(count);
				for (uint32_t i = 0; i < count; i++) {
					OUTCOME_TRY(auto t, r.consume_u32());
					ev.times.push_back(t);
				}
				TRY(r.consume("}"));
			} else {
				return failure(std::format("EventObject: unknown field '{}' at line {}", kw.text, kw.line));
			}
		}
		TRY(r.consume("}"));
		mdx.event_objects.push_back(std::move(ev));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_collision_shape(MDLReader& r, MDX& mdx) {
		TRY(r.consume("CollisionShape"));
		CollisionShape cs {};
		cs.node.id = -1;
		cs.node.parent_id = -1;
		cs.node.flags = Node::Flags::collision_shape;
		cs.radius = 0.f;
		cs.vertices[0] = glm::vec3(0.f);
		cs.vertices[1] = glm::vec3(0.f);
		OUTCOME_TRY(cs.node.name, r.consume_quoted_string());
		TRY(r.consume("{"));

		while (!r.peek_is("}")) {
			OUTCOME_TRY(auto handled, r.try_parse_node_field(cs.node, mdx.unique_tracks, false));
			if (handled) {
				continue;
			}
			OUTCOME_TRY(auto kw, r.consume());
			if (kw.text == "Box") {
				cs.type = CollisionShape::Shape::Box;
			} else if (kw.text == "Plane") {
				cs.type = CollisionShape::Shape::Plane;
			} else if (kw.text == "Sphere") {
				cs.type = CollisionShape::Shape::Sphere;
			} else if (kw.text == "Cylinder") {
				cs.type = CollisionShape::Shape::Cylinder;
			} else if (kw.text == "Vertices") {
				OUTCOME_TRY(auto count, r.consume_u32());
				TRY(r.consume("{"));
				for (uint32_t i = 0; i < count && i < 2; i++) {
					OUTCOME_TRY(cs.vertices[i], r.consume_vec3());
				}
				TRY(r.consume("}"));
			} else if (kw.text == "BoundsRadius") {
				OUTCOME_TRY(cs.radius, r.consume_f32());
			} else {
				return failure(std::format("CollisionShape: unknown field '{}' at line {}", kw.text, kw.line));
			}
		}
		TRY(r.consume("}"));
		mdx.collision_shapes.push_back(std::move(cs));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_camera(MDLReader& r, MDX& mdx) {
		TRY(r.consume("Camera"));
		Camera cam {};
		OUTCOME_TRY(cam.name, r.consume_quoted_string());
		TRY(r.consume("{"));
		while (!r.peek_is("}")) {
			OUTCOME_TRY(auto kw, r.consume());
			if (kw.text == "Position") {
				OUTCOME_TRY(cam.position, r.consume_vec3());
			} else if (kw.text == "Translation") {
				OUTCOME_TRY(cam.KCTR, r.parse_animated_track<glm::vec3>(mdx.unique_tracks));
			} else if (kw.text == "Rotation") {
				OUTCOME_TRY(cam.KCRL, r.parse_animated_track<float>(mdx.unique_tracks));
			} else if (kw.text == "FieldOfView") {
				OUTCOME_TRY(cam.field_of_view, r.consume_f32());
			} else if (kw.text == "FarClip") {
				OUTCOME_TRY(cam.far_clip, r.consume_f32());
			} else if (kw.text == "NearClip") {
				OUTCOME_TRY(cam.near_clip, r.consume_f32());
			} else if (kw.text == "Target") {
				TRY(r.consume("{"));
				while (!r.peek_is("}")) {
					OUTCOME_TRY(auto tkw, r.consume());
					if (tkw.text == "Position") {
						OUTCOME_TRY(cam.target_position, r.consume_vec3());
					} else if (tkw.text == "Translation") {
						OUTCOME_TRY(cam.KTTR, r.parse_animated_track<glm::vec3>(mdx.unique_tracks));
					} else {
						return failure(std::format("Camera Target: unknown field '{}' at line {}", tkw.text, tkw.line));
					}
				}
				TRY(r.consume("}"));
			} else {
				return failure(std::format("Camera: unknown field '{}' at line {}", kw.text, kw.line));
			}
		}
		TRY(r.consume("}"));
		mdx.cameras.push_back(std::move(cam));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_bind_pose(MDLReader& r, MDX& mdx) {
		TRY(r.consume("BindPose"));
		TRY(r.consume("{"));
		TRY(r.consume("Matrices"));
		OUTCOME_TRY(auto count, r.consume_u32());
		TRY(r.consume("{"));
		mdx.bind_poses.reserve(count * 12);
		for (uint32_t i = 0; i < count; i++) {
			TRY(r.consume("{"));
			for (int j = 0; j < 12; j++) {
				OUTCOME_TRY(auto f, r.consume_f32());
				mdx.bind_poses.push_back(f);
			}
			TRY(r.consume("}"));
		}
		TRY(r.consume("}"));
		TRY(r.consume("}"));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_facefx(MDLReader& r, MDX& mdx) {
		TRY(r.consume("FaceFX"));
		FaceFX fx {};
		OUTCOME_TRY(fx.name, r.consume_quoted_string());
		TRY(r.consume("{"));
		while (!r.peek_is("}")) {
			OUTCOME_TRY(auto kw, r.consume());
			if (kw.text == "Path") {
				OUTCOME_TRY(auto p, r.consume_quoted_string());
				fx.path = p;
			} else {
				return failure(std::format("FaceFX: unknown field '{}' at line {}", kw.text, kw.line));
			}
		}
		TRY(r.consume("}"));
		mdx.facefxes.push_back(std::move(fx));
		return outcome::success();
	}

	static outcome::result<void, std::string> parse_corn(MDLReader& r, MDX& mdx) {
		TRY(r.consume("ParticleEmitterPopcorn"));
		CornEmitter c {};
		c.node.id = -1;
		c.node.parent_id = -1;
		c.node.flags = 0;
		OUTCOME_TRY(c.node.name, r.consume_quoted_string());
		TRY(r.consume("{"));
		while (!r.peek_is("}")) {
			OUTCOME_TRY(auto handled, r.try_parse_node_field(c.node, mdx.unique_tracks, false));
			if (handled) {
				continue;
			}
			const auto& tok = r.peek();
			return failure(std::format("ParticleEmitterPopcorn: unknown field '{}' at line {}", tok.text, tok.line));
		}
		TRY(r.consume("}"));
		mdx.corn_emitters.push_back(std::move(c));
		return outcome::success();
	}

	result<MDX, std::string> MDX::from_mdl(std::string_view mdl) {
		OUTCOME_TRY(auto tokens, tokenize(mdl));

		MDLReader reader;
		reader.tokens = std::move(tokens);

		MDX mdx;
		OUTCOME_TRY(parse_version(reader, mdx));

		while (!reader.eof()) {
			const auto& kw = reader.peek();
			if (kw.text == "Model") {
				OUTCOME_TRY(parse_model(reader, mdx));
			} else if (kw.text == "Sequences") {
				OUTCOME_TRY(parse_sequences(reader, mdx));
			} else if (kw.text == "GlobalSequences") {
				OUTCOME_TRY(parse_global_sequences(reader, mdx));
			} else if (kw.text == "Textures") {
				OUTCOME_TRY(parse_textures(reader, mdx));
			} else if (kw.text == "Materials") {
				OUTCOME_TRY(parse_materials(reader, mdx));
			} else if (kw.text == "TextureAnims") {
				OUTCOME_TRY(parse_texture_anims(reader, mdx));
			} else if (kw.text == "Geoset") {
				OUTCOME_TRY(parse_geoset(reader, mdx));
			} else if (kw.text == "GeosetAnim") {
				OUTCOME_TRY(parse_geoset_anim(reader, mdx));
			} else if (kw.text == "Bone") {
				OUTCOME_TRY(parse_bone(reader, mdx));
			} else if (kw.text == "Helper") {
				OUTCOME_TRY(parse_helper(reader, mdx));
			} else if (kw.text == "Light") {
				OUTCOME_TRY(parse_light(reader, mdx));
			} else if (kw.text == "Attachment") {
				OUTCOME_TRY(parse_attachment(reader, mdx));
			} else if (kw.text == "PivotPoints") {
				OUTCOME_TRY(parse_pivots(reader, mdx));
			} else if (kw.text == "ParticleEmitter") {
				OUTCOME_TRY(parse_emitter1(reader, mdx));
			} else if (kw.text == "ParticleEmitter2") {
				OUTCOME_TRY(parse_emitter2(reader, mdx));
			} else if (kw.text == "RibbonEmitter") {
				OUTCOME_TRY(parse_ribbon(reader, mdx));
			} else if (kw.text == "EventObject") {
				OUTCOME_TRY(parse_event_object(reader, mdx));
			} else if (kw.text == "CollisionShape") {
				OUTCOME_TRY(parse_collision_shape(reader, mdx));
			} else if (kw.text == "Camera") {
				OUTCOME_TRY(parse_camera(reader, mdx));
			} else if (kw.text == "BindPose") {
				OUTCOME_TRY(parse_bind_pose(reader, mdx));
			} else if (kw.text == "FaceFX") {
				OUTCOME_TRY(parse_facefx(reader, mdx));
			} else if (kw.text == "ParticleEmitterPopcorn") {
				OUTCOME_TRY(parse_corn(reader, mdx));
			} else {
				return failure(std::format("Unknown top-level section '{}' at line {}", kw.text, kw.line));
			}
		}

		return mdx;
	}
} // namespace mdx
