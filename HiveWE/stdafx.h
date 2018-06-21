#pragma once

#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <unordered_set>
#include <bitset>
#include <set>
#include <memory>
#include <unordered_map>
#include <sstream>
#include <queue>
#include <string>
#include <variant>
#include <regex>
#include <random>
#include <cctype>

using namespace std::literals::string_literals;
namespace fs = std::filesystem;

// Qt headers
#include <QtWidgets>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QSettings>
#include <FlowLayout.h>
#include <QDialogButtonBox>

// Dependencies
#include <glm.hpp>
#define GLM_FORCE_RADIANS
#include <gtc/matrix_transform.hpp>
#include <SOIL2.h>
#include <turbojpeg.h>

// Custom Qt Widgets
#include "ColorButton.h"

#undef _DLL
#include <StormLib.h>

// Base Classes
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "InputHandler.h"

// File formats
#include "MPQ.h"
#include "BLP.h"
#include "INI.h"
#include "SLK.h"
#include "MDX.h"

#include "Utilities.h"
#include "Quadtree.h"

// Resource types
#include "Hierarchy.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "GPUTexture.h"
#include "GroundTexture.h"
#include "CliffMesh.h"
#include "Shader.h"
#include "StaticMesh.h"

#include "TriggerStrings.h"
#include "Imports.h"
#include "Terrain.h"
#include "PathingMap.h"
#include "ShadowMap.h"
#include "Doodads.h"
#include "Units.h"
#include "Brush.h"
#include "PathingBrush.h"
#include "TerrainBrush.h"
#include "MapInfo.h"
#include "Map.h"

#include "Camera.h"
#include "HiveWE.h"
#include "GLWidget.h"

#include "ImportManagerEdit.h"
#include "ImportManager.h"
#include "MapInfoEditor.h"
#include "TriggerEditor.h"
#include "PathingPallete.h"
#include "TerrainPalette.h"
#include "TilePicker.h"
#include "TileSetter.h"
#include "TilePather.h"