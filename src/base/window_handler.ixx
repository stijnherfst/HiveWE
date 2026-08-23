module;

#include <QWidget>

export module WindowHandler;

import std;

export class WindowHandler : public QObject {
	Q_OBJECT
	std::vector<std::pair<std::string, QWidget*>> windows;

  public:

	/// Creates a window of type T if one doesn't exist yet. Otherwise it raises to the foreground and activates the window.
	/// Any arguments after `created` are forwarded to T's constructor, which lets the caller inject the map data
	/// the window works on instead of the window reaching for the map global. They are ignored when a window
	/// already exists, so only pass things that stay valid for as long as the window is open.
	template <typename T, typename... Args>
	T* create_or_raise(QWidget* parent, bool& created, Args&&... args) {
		const auto found = std::find_if(windows.begin(), windows.end(), [&](const auto& item) { return item.first == typeid(T).name(); });
		if (found != windows.end()) {
			T* window = dynamic_cast<T*>(found->second);
			window->raise();
			window->activateWindow();
			created = false;
			return window;
		} else {
			T* window = new T(parent, std::forward<Args>(args)...);
			windows.emplace_back(typeid(T).name(), dynamic_cast<QWidget*>(window));
			connect(window, &T::destroyed, [this, window] {
				std::erase_if(windows, [&](const auto& item) { return item.second == window; });
			});
			created = true;
			return window;
		}
	}

	template <typename T>
	std::optional<T*> get_open() {
		const auto found = std::find_if(windows.begin(), windows.end(), [&](const auto& item) { return item.first == typeid(T).name(); });
		if (found != windows.end()) {
			return dynamic_cast<T*>(found->second);
		} else {
			return std::nullopt;
		}
	}

	void close_all() {
		for (const auto& [name, window] : windows) {
			window->close();
		}
	}
};

export inline WindowHandler window_handler;

#include "window_handler.moc"