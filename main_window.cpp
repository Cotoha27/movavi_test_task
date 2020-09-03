#include "main_window.h"

#include <QFileDialog>
#include <QLabel>
#include <QLayout>
#include <QMainWindow>
#include <QMenuBar>
#include <QScrollArea>

#include <memory>

class MainWindow::Impl : public QMainWindow
{
private:
	QLabel* _scene = nullptr;

public:
	Impl()
	{
		CreateLayout();

		this->setMinimumHeight(600);
		this->setMinimumWidth(800);
	}

private:
	void CreateLayout()
	{
		// main menu

		auto fileMenu = new QMenu("File");
		{
			auto openAction = new QAction("Open");
			connect(openAction, &QAction::triggered, this, &Impl::OnFileMenuOpen);

			fileMenu->addAction(openAction);
		}

		auto menuBar = new QMenuBar();
		menuBar->addMenu(fileMenu);

		// central widget

		//auto controlWidget = new QWidget();
		//controlWidget->setFixedHeight(25);
		//controlWidget->setStyleSheet("background-color:blue;");

		//auto controlLayout = new QHBoxLayout();
		//controlLayout->addWidget(controlWidget);

		_scene = new QLabel();
		_scene->setAlignment(Qt::AlignLeft | Qt::AlignTop);

		auto imageArea = new QScrollArea();
		imageArea->setWidget(_scene);
		imageArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		imageArea->setWidgetResizable(true);

		auto mainLayout = new QVBoxLayout();
		mainLayout->setContentsMargins({ 0, 0, 0, 0 });
		//mainLayout->addLayout(controlLayout);
		mainLayout->addWidget(imageArea);

		auto mainWidget = new QWidget();
		mainWidget->setLayout(mainLayout);

		this->setMenuBar(menuBar);
		this->setCentralWidget(mainWidget);
	}

private slots:
	void OnFileMenuOpen()
	{
		const QString path = QFileDialog::getOpenFileName(0, "Open Image", "", "*.jpg *.png");
		if (path.isEmpty())
		{
			_scene->setPixmap(QPixmap());
		}
		else
		{
			_scene->setPixmap(QPixmap(path));
		}
	}
};

MainWindow::MainWindow()
	: _impl(new Impl())
{ }

MainWindow::~MainWindow()
{
	delete _impl;
}

void MainWindow::Show()
{
	_impl->show();
}
