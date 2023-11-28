module;

#include <string>
#include <vector>
#include <iostream>
#include <optional>
#include <charconv>
#include <unordered_set>
#include <format>

#include <glm/glm.hpp>

#include <outcome/outcome.hpp>

module MDX;

using OUTCOME_V2_NAMESPACE::failure;
using OUTCOME_V2_NAMESPACE::result;

namespace mdx {
#define TRY(r)                                \
	{                                         \
		if (auto optional = r; optional) {    \
			return failure(optional.value()); \
		}                                     \
	}

#define TRY_PASS(r)          \
	{                        \
		auto optional = r;   \
		if (optional) {      \
			return optional; \
		}                    \
	}

	struct MDLReader {
		std::vector<std::string_view> tokens;
		size_t position = 0;

		std::string_view current() {
			return tokens[position];
		}

		std::optional<std::string> consume(std::string_view token) {
			if (position >= tokens.size()) [[unlikely]] {
				return std::format("Expected: {}, but we reached the end of the file", token);
			}
			if (tokens[position] != token) [[unlikely]] {
				return std::format("Expected: {}, got: {}", token, tokens[position]);
			}
			position += 1;
			return std::nullopt;
		}

		template <typename F>
		std::optional<std::string> consume_any(F callback) {
			if (position >= tokens.size()) [[unlikely]] {
				 return std::format("Expected a name/value, but we reached the end of the file");
			}
			std::optional<std::string> error = callback(tokens[position]);
			position += 1;
			if (error) [[unlikely]] {
				return error;
			}
			return std::nullopt;
		}

		template <typename F>
		std::optional<std::string> consume_quoted_string(F callback) {
			auto token = current();
			if (position >= tokens.size()) [[unlikely]] {
				return std::format("Expected quoted string (e.g. \"SomeName\"), but we reached the end of the file");
			}

			if (token.size() < 3 || token.front() != '\"' || token.back() != '\"') [[unlikely]] {
				return std::format("The name should be surrounded in quotes \"likethis\" and at least one character long, but is: {}", token);
			}

			callback(token.substr(1, token.size() - 2)); // Get rid of quotes
			position += 1;
			return std::nullopt;
		}

		template <typename F>
		std::optional<std::string> consume_attributes(
			const std::unordered_set<std::string_view>& attributes,
			const std::unordered_set<std::string_view>& chunks,
			F callback
		) {
			//if (one_of_each) {
			//	std::unordered_set<std::string> already_seen;

			//}
			
			/*while (current() != "}") {
				
			}*/

			if (attributes.contains(current())) {
				position += 1;
				if (current() == "{") {
					
				}
			}
		}
	};

	std::optional<std::string> parse_version_chunk(MDLReader& reader, MDX& mdx) {
		TRY_PASS(reader.consume("Version"));
		TRY_PASS(reader.consume("{"));
		TRY_PASS(reader.consume("FormatVersion"));

		TRY_PASS(reader.consume_any([&](std::string_view token) -> std::optional<std::string> {
			std::from_chars(token.data(), token.data() + token.size(), mdx.version);
			if (mdx.version != 800 || mdx.version != 900 || mdx.version != 1000) [[unlikely]] {
				return std::format("Invalid version {}, expected 800, 900, 1000", mdx.version);
			}
			return std::nullopt;
		}));

		TRY_PASS(reader.consume("}"));
		return std::nullopt;
	}

	std::optional<std::string> parse_model_chunk(MDLReader& reader, MDX& mdx) {
		TRY_PASS(reader.consume("Model"));

		TRY_PASS(reader.consume_quoted_string([&](std::string_view token) -> std::optional<std::string> {
			mdx.name = std::string(token);			
			return std::nullopt;
		}));

		TRY_PASS(reader.consume("{"));

		while (reader.current() != "}") {
			if (reader.current() == "BlendTime") {
				
				/*mdx.blend_time = 
				name = reader.read_string(80);
				animation_filename = reader.read_string(260);
				extent = Extent(reader);
				blend_time = reader.read<uint32_t>();*/
			}
		}

		TRY_PASS(reader.consume("}"));
		return std::nullopt;
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

			std::cout << mdl.substr(0, pos) << "\n";

			pos = mdl.find_first_not_of(" \t\r\n,", pos);
			if (pos == std::string::npos) {
				break;
			}
			mdl.remove_prefix(pos);
		}

		MDX mdx;
		TRY(parse_version_chunk(reader, mdx));
		while (!reader.tokens.empty()) {
			if (reader.current() == "Model") {
				TRY(parse_model_chunk(reader, mdx));
			}

			return failure(std::format("Error reading token {}, expected a chunk type (Version/Model/Textures/etc). Make sure to match the casing", reader.current()));
		}

		return mdx;
	}

	//void MDX::from_mdl2(std::string_view mdl) {
		// tao::pegtl::string_input input(mdl.substr(0), "from_content");
		// tao::pegtl::parse<file, my_action>(input);
	//}
} // namespace mdx