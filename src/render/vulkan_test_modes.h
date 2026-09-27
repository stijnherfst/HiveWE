#pragma once

/// Opens a Vulkan model grid and single preview of stock game models instead of the editor.
/// Needs the game data to be open. Returns the application's exit code.
int run_vulkan_model_grid_test();

/// Opens the Vulkan model editor on a stock game model instead of the editor.
/// Needs the game data to be open. Returns the application's exit code.
int run_vulkan_model_editor_test();
