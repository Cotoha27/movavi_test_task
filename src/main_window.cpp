#include "main_window.h"

#include "pyramid_image_proc.h"

#include <QComboBox>
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

	QComboBox* _fileCombobox = nullptr;
	bool _needAddFileToCombobox = false;

	QComboBox* _layerCombobox = nullptr;

	PyramidImageProc _pyramidProcessor;

public:
	Impl()
	{
		CreateLayout();

		this->setMinimumHeight(600);
		this->setMinimumWidth(800);

		connect(&_pyramidProcessor, &PyramidImageProc::ImageLoaded, this, &Impl::OnPyramidImageLoaded);
		connect(&_pyramidProcessor, &PyramidImageProc::LayerChanged, this, &Impl::OnPyramidLayerChanged);
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

		// control widget

		auto controlLayout = new QHBoxLayout();
		controlLayout->setContentsMargins(10, 5, 0, 0);
		controlLayout->setAlignment(Qt::AlignLeft);
		{
			auto fileLabel = new QLabel();
			fileLabel->setText("File:");

			_fileCombobox = new QComboBox();
			_fileCombobox->setFixedWidth(500);
			connect(_fileCombobox, &QComboBox::currentTextChanged, this, &Impl::OnFileComboboxCurrentTextChanged);

			auto layerLabel = new QLabel();
			layerLabel->setContentsMargins(10, 0, 0, 0);
			layerLabel->setText("Layer:");

			_layerCombobox = new QComboBox();
			_layerCombobox->setFixedWidth(40);
			connect(_layerCombobox, qOverload<int>(&QComboBox::currentIndexChanged), this, &Impl::OnLayerComboboxCurrentIndexChanged);

			auto sizeCaptionLabel = new QLabel();
			sizeCaptionLabel->setContentsMargins(10, 0, 0, 0);
			sizeCaptionLabel->setText("Size:");

			controlLayout->addWidget(fileLabel, 0, Qt::AlignVCenter);
			controlLayout->addWidget(_fileCombobox, 0, Qt::AlignVCenter);
			controlLayout->addWidget(layerLabel, 0, Qt::AlignVCenter);
			controlLayout->addWidget(_layerCombobox, 0, Qt::AlignVCenter);
			controlLayout->addWidget(sizeCaptionLabel, 0, Qt::AlignVCenter);
		}

		// central widget

		_scene = new QLabel();
		_scene->setAlignment(Qt::AlignLeft | Qt::AlignTop);

		auto imageArea = new QScrollArea();
		imageArea->setWidget(_scene);
		imageArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		imageArea->setWidgetResizable(true);

		auto mainLayout = new QVBoxLayout();
		mainLayout->setContentsMargins({ 0, 0, 0, 0 });
		mainLayout->addLayout(controlLayout);
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
		if (!path.isEmpty())
		{
			_needAddFileToCombobox = true;
			this->setEnabled(false);
			_pyramidProcessor.BeginLoadingImage(path);
		}
	}

	void OnFileComboboxCurrentTextChanged(const QString& text)
	{
		this->setEnabled(false);
		_pyramidProcessor.BeginLoadingImage(text);
	}

	void OnLayerComboboxCurrentIndexChanged(int index)
	{
		this->setEnabled(false);
		_pyramidProcessor.BeginChangeLayer(index);
	}

	void OnPyramidImageLoaded(const QString& path, const QPixmap& image, int layersCount)
	{
		_fileCombobox->blockSignals(true);
		_layerCombobox->blockSignals(true);

		_layerCombobox->clear();

		if (image.isNull())
		{
			_scene->setPixmap(QPixmap());
		}
		else
		{
			if (_needAddFileToCombobox)
			{
				_fileCombobox->addItem(path);
				_needAddFileToCombobox = false;
			}

			_fileCombobox->setCurrentText(path);

			for (int i = 0; i < layersCount; i++)
			{
				_layerCombobox->addItem(QString::number(i));
			}
			_layerCombobox->setCurrentIndex(0);

			_scene->setPixmap(image);
		}

		_layerCombobox->blockSignals(false);
		_fileCombobox->blockSignals(false);

		this->setEnabled(true);
	}

	void OnPyramidLayerChanged(const QPixmap& image)
	{
		_scene->setPixmap(image);
		this->setEnabled(true);
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
