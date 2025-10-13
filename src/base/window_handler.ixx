module;

#include <QWidget>

export module WindowHandler;

import std;

export class WindowHandler : public QObject {
	Q_OBJECT
	std::vector<std::pair<std::string, QWidget*>> windows;

  public:

	/// Creates a window of type T if one doesn't exist yet. Otherwise it raises to the foreground and activates the window.
	template <typename T>
	T* create_or_raise(QWidget* parent, bool& created) {
		const auto found = std::find_if(windows.begin(), windows.end(), [&](const auto& item) { return item.first == typeid(T).name(); });
		if (found != windows.end()) {
			T* window = dynamic_cast<T*>(found->second);
			window->raise();
			window->activateWindow();
			created = false;
			return window;
		} else {
			T* window = new T(parent);
			windows.emplace_back(typeid(T).name(), dynamic_cast<QWidget*>(window));
			connect(window, &T::destroyed, [this, window] {
				const auto found = std::find_if(windows.begin(), windows.end(), [&](const auto& item) { return item.second == window; });
				windows.erase(found);
			});
			created = true;
			return window;
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