#pragma once


class WindowHandler : public QObject {
	Q_OBJECT

public:
	std::vector<std::pair<std::string, QWidget*>> windows;

	/// Creates a window of type T if one doesn't exist yet. Otherwise it raises to the foreground and activates the window.
	template<typename T>
	T* create_or_raise() {
		auto found = std::find_if(windows.begin(), windows.end(), [&](const auto& item) { return item.first == typeid(T).name(); });
		if (found != windows.end()) {
			T* window = dynamic_cast<T*>((*found).second);
			window->raise();
			window->activateWindow();
			return window;
		} else {
			T* window = new T();
			windows.push_back(std::make_pair(typeid(T).name(), dynamic_cast<QWidget*>(window)));
			connect(window, &T::destroyed, [this, window]() { 
				auto found = std::find_if(windows.begin(), windows.end(), [&](const auto& item) { return item.second == window; });
				windows.erase(found);
			});
			return window;
		}
		return nullptr;
	}
};