#include "imgui_renderer.h"

#include <QDateTime>
#include <QFile>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QClipboard>
#include <QCursor>

namespace QtImGui {

namespace {

// Keyboard mapping. Dear ImGui use those indices to peek into the io.KeysDown[] array.
const QHash<int, int> keyMap = {
    { Qt::Key_Tab, ImGuiKey_Tab },
    { Qt::Key_Left, ImGuiKey_LeftArrow },
    { Qt::Key_Right, ImGuiKey_RightArrow },
    { Qt::Key_Up, ImGuiKey_UpArrow },
    { Qt::Key_Down, ImGuiKey_DownArrow },
    { Qt::Key_PageUp, ImGuiKey_PageUp },
    { Qt::Key_PageDown, ImGuiKey_PageDown },
    { Qt::Key_Home, ImGuiKey_Home },
    { Qt::Key_End, ImGuiKey_End },
    { Qt::Key_Insert, ImGuiKey_Insert },
    { Qt::Key_Delete, ImGuiKey_Delete },
    { Qt::Key_Backspace, ImGuiKey_Backspace },
    { Qt::Key_Space, ImGuiKey_Space },
    { Qt::Key_Enter, ImGuiKey_Enter },
    { Qt::Key_Return, ImGuiKey_Enter },
    { Qt::Key_Escape, ImGuiKey_Escape },
    { Qt::Key_A, ImGuiKey_A },
    { Qt::Key_C, ImGuiKey_C },
    { Qt::Key_V, ImGuiKey_V },
    { Qt::Key_X, ImGuiKey_X },
    { Qt::Key_Y, ImGuiKey_Y },
    { Qt::Key_Z, ImGuiKey_Z },
    { Qt::MiddleButton, ImGuiMouseButton_Middle }
};

#ifndef QT_NO_CURSOR
const QHash<ImGuiMouseCursor, Qt::CursorShape> cursorMap = {
    { ImGuiMouseCursor_Arrow,      Qt::CursorShape::ArrowCursor },
    { ImGuiMouseCursor_TextInput,  Qt::CursorShape::IBeamCursor },
    { ImGuiMouseCursor_ResizeAll,  Qt::CursorShape::SizeAllCursor },
    { ImGuiMouseCursor_ResizeNS,   Qt::CursorShape::SizeVerCursor },
    { ImGuiMouseCursor_ResizeEW,   Qt::CursorShape::SizeHorCursor },
    { ImGuiMouseCursor_ResizeNESW, Qt::CursorShape::SizeBDiagCursor },
    { ImGuiMouseCursor_ResizeNWSE, Qt::CursorShape::SizeFDiagCursor },
    { ImGuiMouseCursor_Hand,       Qt::CursorShape::PointingHandCursor },
    { ImGuiMouseCursor_NotAllowed, Qt::CursorShape::ForbiddenCursor },
};
#endif

QByteArray g_currentClipboardText;

} // namespace

void ImGuiRenderer::applyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Spacing & rounding for a softer, modern look
    style.WindowPadding     = ImVec2(8.0f, 8.0f);
    style.FramePadding      = ImVec2(8.0f, 4.0f);
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing  = ImVec2(6.0f, 6.0f);
    style.IndentSpacing     = 22.0f;
    style.ScrollbarSize     = 14.0f;
    style.GrabMinSize       = 12.0f;

    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.TabBorderSize     = 0.0f;

    style.WindowRounding    = 6.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 4.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 4.0f;

    style.WindowTitleAlign  = ImVec2(0.5f, 0.5f);

    // Theme based on the HiveWE scroll mascot: an azure-blue to teal/cyan gradient
    // with warm brown accents (the eyes/eyebrows).
    const ImVec4 blue       = ImVec4(0.12f, 0.63f, 0.91f, 1.00f); // scroll top
    const ImVec4 blueBright = ImVec4(0.22f, 0.73f, 1.00f, 1.00f);
    const ImVec4 teal       = ImVec4(0.09f, 0.80f, 0.69f, 1.00f); // scroll bottom
    const ImVec4 tealDeep   = ImVec4(0.08f, 0.55f, 0.49f, 1.00f);
    const ImVec4 brown      = ImVec4(0.42f, 0.25f, 0.08f, 1.00f); // eyes

    colors[ImGuiCol_Text]                  = ImVec4(0.90f, 0.94f, 0.96f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.45f, 0.54f, 0.57f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.07f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.08f, 0.12f, 0.14f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.16f, 0.31f, 0.36f, 0.60f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.12f, 0.17f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.14f, 0.24f, 0.29f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.12f, 0.30f, 0.36f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.06f, 0.09f, 0.11f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.08f, 0.20f, 0.26f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.06f, 0.09f, 0.11f, 0.75f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.09f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.06f, 0.09f, 0.11f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.16f, 0.30f, 0.34f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.18f, 0.40f, 0.44f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = tealDeep;
    colors[ImGuiCol_CheckMark]             = teal;
    colors[ImGuiCol_SliderGrab]            = blue;
    colors[ImGuiCol_SliderGrabActive]      = teal;
    colors[ImGuiCol_Button]                = ImVec4(0.13f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = blue;
    colors[ImGuiCol_ButtonActive]          = teal;
    colors[ImGuiCol_Header]                = ImVec4(0.13f, 0.24f, 0.29f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.12f, 0.63f, 0.91f, 0.55f);
    colors[ImGuiCol_HeaderActive]          = blue;
    colors[ImGuiCol_Separator]             = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.12f, 0.63f, 0.91f, 0.55f);
    colors[ImGuiCol_SeparatorActive]       = teal;
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.09f, 0.80f, 0.69f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.09f, 0.80f, 0.69f, 0.55f);
    colors[ImGuiCol_ResizeGripActive]      = teal;
    colors[ImGuiCol_Tab]                   = ImVec4(0.09f, 0.14f, 0.17f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.12f, 0.63f, 0.91f, 0.55f);
    colors[ImGuiCol_TabSelected]           = ImVec4(0.10f, 0.32f, 0.40f, 1.00f);
    colors[ImGuiCol_TabDimmed]             = ImVec4(0.07f, 0.11f, 0.13f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected]     = ImVec4(0.09f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_PlotLines]             = blue;
    colors[ImGuiCol_PlotLinesHovered]      = teal;
    colors[ImGuiCol_PlotHistogram]         = teal;
    colors[ImGuiCol_PlotHistogramHovered]  = blueBright;
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.12f, 0.63f, 0.91f, 0.35f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(brown.x + 0.20f, brown.y + 0.15f, brown.z + 0.08f, 0.90f); // warm brown highlight
    colors[ImGuiCol_NavCursor]             = teal;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.00f, 0.00f, 0.00f, 0.55f);
}

void ImGuiRenderer::initialize(WindowWrapper *window) {
    m_window.reset(window);

    g_ctx = ImGui::CreateContext();
    ImGui::SetCurrentContext(g_ctx);

    applyTheme();

    // Setup backend capabilities flags
    ImGuiIO &io = ImGui::GetIO();
    #ifndef QT_NO_CURSOR
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;  // We can honor io.WantSetMousePos requests (optional, rarely used)
    #endif
    io.BackendPlatformName = "qtimgui";
    
    // Setup keyboard mapping
    // for (auto key : keyMap.values()) {
    //     io.KeyMap[key] = key;
    // }
    
    // io.RenderDrawListsFn = [](ImDrawData *drawData) {
    //    instance()->renderDrawList(drawData);
    // };
    io.SetClipboardTextFn = [](void *user_data, const char *text) {
        Q_UNUSED(user_data);
        QGuiApplication::clipboard()->setText(text);
    };
    io.GetClipboardTextFn = [](void *user_data) {
        Q_UNUSED(user_data);
        g_currentClipboardText = QGuiApplication::clipboard()->text().toUtf8();
        return (const char *)g_currentClipboardText.data();
    };

    window->installEventFilter(this);
}

void ImGuiRenderer::setFontUploader(FontUploader uploader) {
    m_fontUploader = std::move(uploader);
}

void ImGuiRenderer::createFontsTexture()
{
    // Select current context
    ImGui::SetCurrentContext(g_ctx);

    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();

    // Replace the default bitmap font with a proportional, anti-aliased UI font.
    // Rasterize at the display's pixel ratio so text stays crisp on high-DPI screens,
    // then scale glyphs back down so layout stays in logical units.
    float dpr = m_window ? static_cast<float>(m_window->devicePixelRatio()) : 1.0f;
    if (dpr < 1.0f) {
        dpr = 1.0f;
    }

	ImFontConfig config;
    config.OversampleH = 2;
    config.OversampleV = 1;
    config.PixelSnapH = false;

    QString fonts_dir = QString::fromLocal8Bit(qgetenv("SystemRoot"));
    if (fonts_dir.isEmpty()) {
        fonts_dir = "C:/Windows";
    }
    fonts_dir += "/Fonts/";

	const ImFont* font = nullptr;
    for (const char* candidate : { "segoeui.ttf", "tahoma.ttf", "arial.ttf" }) {
        const QString path = fonts_dir + candidate;
        if (QFile::exists(path)) {
			constexpr float font_size = 16.0f;
			font = io.Fonts->AddFontFromFileTTF(path.toUtf8().constData(), font_size * dpr, &config);
            break;
        }
    }
    if (!font) {
        io.Fonts->AddFontDefault();
    } else {
        // Font is baked at native pixels; scale glyphs back to logical units (moved from io.FontGlobalScale in 1.92).
        ImGui::GetStyle().FontScaleMain = 1.0f / dpr;
    }

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    if (m_fontUploader) {
        io.Fonts->TexID = m_fontUploader(pixels, width, height);
    }
    g_FontsCreated = true;
}

void ImGuiRenderer::newFrame()
{
    // Select current context
    ImGui::SetCurrentContext(g_ctx);

    if (!g_FontsCreated)
        createFontsTexture();

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    io.DisplaySize = ImVec2(m_window->size().width(), m_window->size().height());
    io.DisplayFramebufferScale = ImVec2(m_window->devicePixelRatio(), m_window->devicePixelRatio());

    // Setup time step
    double current_time =  QDateTime::currentMSecsSinceEpoch() / double(1000);
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f/60.0f);
    if (io.DeltaTime <= 0.0f) io.DeltaTime = 0.00001f;
    g_Time = current_time;
    
    
    // If ImGui wants to set cursor position (for example, during navigation by using keyboard)
    // we need to do it here (before getting `QCursor::pos()` below).
    setCursorPos(io);

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    if (m_window->isActive())
    {
        const QPoint pos = m_window->mapFromGlobal(QCursor::pos());
        io.MousePos = ImVec2(pos.x(), pos.y());   // Mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
    }
    else
    {
        io.MousePos = ImVec2(-1,-1);
    }

    for (int i = 0; i < 3; i++)
    {
        io.MouseDown[i] = g_MousePressed[i];
    }

    io.MouseWheelH = g_MouseWheelH;
    io.MouseWheel = g_MouseWheel;
    g_MouseWheelH = 0;
    g_MouseWheel = 0;

    
    updateCursorShape(io);
    

    // Start the frame
    ImGui::NewFrame();
}

ImGuiRenderer::ImGuiRenderer()
  : g_ctx(nullptr)
{
}

ImGuiRenderer::~ImGuiRenderer()
{
  // remove this context
  ImGui::DestroyContext(g_ctx);
}

void ImGuiRenderer::onMousePressedChange(QMouseEvent *event)
{
    g_MousePressed[0] = event->buttons() & Qt::LeftButton;
    g_MousePressed[1] = event->buttons() & Qt::RightButton;
    g_MousePressed[2] = event->buttons() & Qt::MiddleButton;
}

void ImGuiRenderer::onWheel(QWheelEvent *event)
{
    // Select current context
    ImGui::SetCurrentContext(g_ctx);

    // Handle horizontal component
    if(event->pixelDelta().x() != 0)
    {
        g_MouseWheelH += event->pixelDelta().x() / (ImGui::GetTextLineHeight());
    } else {
        // Magic number of 120 comes from Qt doc on QWheelEvent::pixelDelta()
        g_MouseWheelH += event->angleDelta().x() / 120.0f;
    }

    // Handle vertical component
    if(event->pixelDelta().y() != 0)
    {
        // 5 lines per unit
        g_MouseWheel += event->pixelDelta().y() / (5.0 * ImGui::GetTextLineHeight());
    } else {
        // Magic number of 120 comes from Qt doc on QWheelEvent::pixelDelta()
        g_MouseWheel += event->angleDelta().y() / 120.0f;
    }
}

void ImGuiRenderer::onKeyPressRelease(QKeyEvent *event)
{
    // Select current context
    ImGui::SetCurrentContext(g_ctx);

    ImGuiIO& io = ImGui::GetIO();
    
    const bool key_pressed = (event->type() == QEvent::KeyPress);
    
    // Translate `Qt::Key` into `ImGuiKey`, and apply 'pressed' state for that key
    const auto key_it = keyMap.constFind( event->key() );
    if (key_it != keyMap.constEnd()) { // Qt's key found in keyMap
        const int imgui_key = *(key_it);
    	io.AddKeyEvent(static_cast<ImGuiKey>(imgui_key), key_pressed);
        // io.KeysDown[imgui_key] = key_pressed;
    }

    if (key_pressed) {
        const QString text = event->text();
        if (text.size() == 1) {
            io.AddInputCharacter( text.at(0).unicode() );
        }
    }

#ifdef Q_OS_MAC
    io.KeyCtrl  = event->modifiers() & Qt::MetaModifier;
    io.KeyShift = event->modifiers() & Qt::ShiftModifier;
    io.KeyAlt   = event->modifiers() & Qt::AltModifier;
    io.KeySuper = event->modifiers() & Qt::ControlModifier; // Comamnd key
#else
    io.KeyCtrl  = event->modifiers() & Qt::ControlModifier;
    io.KeyShift = event->modifiers() & Qt::ShiftModifier;
    io.KeyAlt   = event->modifiers() & Qt::AltModifier;
    io.KeySuper = event->modifiers() & Qt::MetaModifier;
#endif
}

void ImGuiRenderer::updateCursorShape(const ImGuiIO& io)
{
    // NOTE: This code will be executed, only if the following flags have been set:
    // - backend flag: `ImGuiBackendFlags_HasMouseCursors`    - enabled
    // - config  flag: `ImGuiConfigFlags_NoMouseCursorChange` - disabled

#ifndef QT_NO_CURSOR
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;

    const ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || (imgui_cursor == ImGuiMouseCursor_None))
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        m_window->setCursorShape(Qt::CursorShape::BlankCursor);
    }
    else
    {
        // Show OS mouse cursor
        
        // Translate `ImGuiMouseCursor` into `Qt::CursorShape` and show it, if we can
        const auto cursor_it = cursorMap.constFind( imgui_cursor );
        if(cursor_it != cursorMap.constEnd()) // `Qt::CursorShape` found for `ImGuiMouseCursor`
        {
            const Qt::CursorShape qt_cursor_shape = *(cursor_it);
            m_window->setCursorShape(qt_cursor_shape);
        }
        else // shape NOT found - use default
        {
            m_window->setCursorShape(Qt::CursorShape::ArrowCursor);
        }
    }
#else
    Q_UNUSED(io);
#endif
}

void ImGuiRenderer::setCursorPos(const ImGuiIO& io)
{
    // NOTE: This code will be executed, only if the following flags have been set:
    // - backend flag: `ImGuiBackendFlags_HasSetMousePos`      - enabled
    // - config  flag: `ImGuiConfigFlags_NavEnableSetMousePos` - enabled
    
#ifndef QT_NO_CURSOR
    if(io.WantSetMousePos) {
        m_window->setCursorPos({(int)io.MousePos.x, (int)io.MousePos.y});
    }
#else
    Q_UNUSED(io);
#endif
}

bool ImGuiRenderer::eventFilter(QObject *watched, QEvent *event)
{
  if (watched == m_window->object()) {
    switch (event->type()) {
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
      this->onMousePressedChange(static_cast<QMouseEvent*>(event));
      break;
    case QEvent::Wheel:
      this->onWheel(static_cast<QWheelEvent*>(event));
      break;
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
      this->onKeyPressRelease(static_cast<QKeyEvent*>(event));
      break;
    default:
      break;
    }
  }
  return QObject::eventFilter(watched, event);
}

ImGuiRenderer* ImGuiRenderer::instance() {
    static ImGuiRenderer* instance = nullptr;
    if (!instance) {
        instance = new ImGuiRenderer();
    }
    return instance;
}

} // namespace QtImGui
