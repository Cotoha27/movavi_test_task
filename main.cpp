#include "main_window.h"

#include <qapplication.h>

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	MainWindow mainWindow;
	mainWindow.Show();

	return app.exec();
}