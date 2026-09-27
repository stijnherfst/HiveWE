// Shared by the skinned mesh fragment shaders, after skinned_mesh_common.glsl.
// The transparent pass draws in back-to-front order, so its draws can only be merged into one multi-draw when they
// share fixed-function state. The shader takes over back-face culling and blending to make that possible.

/// Bits of pc.shader_state; mirror ShaderState in src/base/render_manager.ixx
const uint shader_cull = 1u;
const uint shader_blend = 2u;

/// Bits of LayerParams.flags; mirror src/resources/skinned_mesh.ixx
const uint layer_blend_mask = 7u;
const uint layer_two_sided = 8u;

/// BlendMode in src/render/vk_editable_mesh.ixx
const uint blend_alpha = 1u;
const uint blend_additive = 2u;
const uint blend_modulate = 3u;

/// Discards the back faces of one-sided layers, for pipelines that cull nothing
void cull_back_face(const LayerParams p) {
	if ((pc.shader_state & shader_cull) != 0u && (p.flags & layer_two_sided) == 0u && !gl_FrontFacing) {
		discard;
	}
}

/// Expresses the layer's blend mode for the dual-source blend equation `color * ONE + destination * factor`.
/// Modulate 2x needs a blend factor of 2, which unorm targets clamp, so those layers keep a fixed-function pipeline.
void blend(const LayerParams p, inout vec4 color, out vec4 factor) {
	factor = vec4(0.f);
	if ((pc.shader_state & shader_blend) == 0u) {
		return;
	}
	switch (p.flags & layer_blend_mask) {
		case blend_alpha: // SRC_ALPHA, ONE_MINUS_SRC_ALPHA
			factor = vec4(1.f - color.a);
			color = vec4(color.rgb * color.a, color.a * color.a);
			break;
		case blend_additive: // SRC_ALPHA, ONE
			factor = vec4(1.f);
			color = vec4(color.rgb * color.a, color.a * color.a);
			break;
		case blend_modulate: // ZERO, SRC_COLOR
			factor = color;
			color = vec4(0.f);
			break;
		// Unblended layers (ONE, ZERO) keep the zero factor
	}
}
