#pragma once

#include <QObject>
#include <QPoint>
#include <imgui.h>
#include <functional>
#include <memory>

class QMouseEvent;
class QWheelEvent;
class QKeyEvent;

namespace QtImGui {

class WindowWrapper {
public:
    virtual ~WindowWrapper() {}
    virtual void installEventFilter(QObject *object) = 0;
    virtual QSize size() const = 0;
    virtual qreal devicePixelRatio() const = 0;
    virtual bool isActive() const = 0;
    virtual QPoint mapFromGlobal(const QPoint &p) const = 0;
    virtual QObject* object() = 0;
    
    virtual void setCursorShape(Qt::CursorShape shape) = 0;
    virtual void setCursorPos(const QPoint& local_pos) = 0;
};

/// Uploads the font atlas (RGBA, 8 bits per channel) to the graphics backend and returns its texture ID
using FontUploader = std::function<ImTextureID(const unsigned char* pixels, int width, int height)>;

/// Feeds Qt input into an ImGui context. Drawing ImGui::GetDrawData() is left to the graphics backend.
class ImGuiRenderer : public QObject {
    Q_OBJECT
public:
    void initialize(WindowWrapper *window);
    void setFontUploader(FontUploader uploader);
    void newFrame();
    bool eventFilter(QObject *watched, QEvent *event);

    static ImGuiRenderer *instance();

public:
    ImGuiRenderer();
    ~ImGuiRenderer();

private:
    void onMousePressedChange(QMouseEvent *event);
    void onWheel(QWheelEvent *event);
    void onKeyPressRelease(QKeyEvent *event);
    
    void updateCursorShape(const ImGuiIO &io);
    void setCursorPos(const ImGuiIO &io);

    void applyTheme();
    void createFontsTexture();

    std::unique_ptr<WindowWrapper> m_window;
    double       g_Time = 0.0f;
    bool         g_MousePressed[3] = { false, false, false };
    float        g_MouseWheel;
    float        g_MouseWheelH;
    bool         g_FontsCreated = false;
    FontUploader m_fontUploader;

    ImGuiContext* g_ctx = nullptr;
};

} // namespace QtImGui
