#pragma once

#include "imgui_renderer.h"

class QWidget;
class QWindow;

namespace QtImGui {

typedef void* RenderRef;

#ifdef QT_WIDGETS_LIB
RenderRef initialize(QWidget *window, bool defaultRender = true);
#endif

RenderRef initialize(QWindow *window, bool defaultRender = true);
/// Sets how the font atlas reaches the graphics backend. Call before the first newFrame().
void setFontUploader(RenderRef ref, FontUploader uploader);
void newFrame(RenderRef ref = nullptr);

}
