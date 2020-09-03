#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

class MainWindow
{
public:
	MainWindow();
	~MainWindow();

	void Show();

private:
	class Impl;
	Impl* const _impl = nullptr;
};

#endif // MAIN_WINDOW_H
