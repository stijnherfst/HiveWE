#pragma once

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

using namespace std::literals::string_literals;
namespace fs = std::experimental::filesystem;

#include <QtWidgets>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <FlowLayout.h>

#include <glm.hpp>
#define GLM_FORCE_RADIANS
#include <gtc/matrix_transform.hpp>
#include <SOIL2.h>
#include <turbojpeg.h>

#undef _DLL
#include <StormLib.h>

#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "InputHandler.h"

// File formats
#include "MPQ.h"
#include "BLP.h"
#include "SLK.h"
#include "INI.h"
#include "MDX.h"

#include "Utilities.h"

// Resource types
#include "Hierarchy.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "CliffMesh.h"
#include "Shader.h"
#include "StaticMesh.h"

#include "Terrain.h"
#include "PathingMap.h"
#include "Doodads.h"
#include "Units.h"
#include "Brush.h"
#include "Map.h"

#include "Camera.h"
#include "HiveWE.h"
#include "GLWidget.h"

#include "PathingPallete.h"
#include "TilePicker.h"
#include "TileSetter.h"