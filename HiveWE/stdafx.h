#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <unordered_set>
#include <bitset>
#include <set>

using namespace std::literals::string_literals;
namespace fs = std::experimental::filesystem;

#include <QtWidgets>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_2_Core>

#include <glm.hpp>
#define GLM_FORCE_RADIANS
#include <gtc/matrix_transform.hpp>
#include <soil2.h>

#undef _DLL
#include <StormLib.h>


#include "Utilities.h"
#include "MPQ.h"
#include "Hierarchy.h"
#include "ResourceManager.h"

#include "Camera.h"
#include "HiveWE.h"
#include "GLWidget.h"
#include "Terrain.h"
#include "Map.h"