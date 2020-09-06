#include "main_window.h"

#include "pyramid_image_proc.h"

#include <QAbstractListModel>
#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QLayout>
#include <QMainWindow>
#include <QMenuBar>
#include <QScrollArea>

#include <QtMath>

#include <memory>

namespace {

struct FileRecord
{
	QString path;
	int diagonal = -1;
};

bool operator< (const FileRecord& lhs, const FileRecord& rhs)
{
	return lhs.diagonal < rhs.diagonal;
};

class ImageFileModel : public QAbstractListModel
{
private:
	QList<FileRecord> _data;

public:
	int Add(const QString& path, const QSize& size)
	{
		FileRecord newRecord;
		{
			const qint64 w = size.width();
			const qint64 h = size.height();

			const int diagonal = qSqrt(w * w + h * h);

			newRecord = std::move(FileRecord{ path, diagonal });
		}

		auto it = std::lower_bound(_data.begin(), _data.end(), newRecord);
		if (it == _data.end() || path != (*it).path)
		{
			it = _data.insert(it, newRecord);
		}

		emit dataChanged(QModelIndex(), QModelIndex());

		return std::distance(_data.begin(), it);
	}

	int IndexOf(const QString& path)
	{
		for (int i = 0; i < _data.size(); i++)
		{
			if (_data[i].path == path)
				return i;
		}

		return -1;
	}

public:
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override
	{
		return _data.size();
	}

	virtual QVariant ImageFileModel::data(const QModelIndex& index, int role) const override
	{
		const FileRecord& data = _data.at(index.row());

		QVariant value;
		if (role == Qt::DisplayRole)
		{
			value = data.path;
		}

		return value;
	}
};

} // mamespace

class MainWindow::Impl : public QMainWindow
{
private:
	QLabel* _scene = nullptr;

	QComboBox* _fileCombobox = nullptr;
	QComboBox* _layerCombobox = nullptr;
	QLabel* _layerSizeLabel = nullptr;

	bool _needAddFileToCombobox = false;

	ImageFileModel _model;

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
			_fileCombobox->setModel(&_model);
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

			_layerSizeLabel = new QLabel();
			_layerSizeLabel->setText("-");

			controlLayout->addWidget(fileLabel, 0, Qt::AlignVCenter);
			controlLayout->addWidget(_fileCombobox, 0, Qt::AlignVCenter);
			controlLayout->addWidget(layerLabel, 0, Qt::AlignVCenter);
			controlLayout->addWidget(_layerCombobox, 0, Qt::AlignVCenter);
			controlLayout->addWidget(sizeCaptionLabel, 0, Qt::AlignVCenter);
			controlLayout->addWidget(_layerSizeLabel, 0, Qt::AlignVCenter);
		}

		// central widget

		_scene = new QLabel();
		_scene->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		_scene->setScaledContents(true);

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

private:
	void SetImage(const QPixmap& image)
	{
		_scene->setPixmap(image);

		if (image.isNull())
		{
			_layerSizeLabel->setText("-");
		}
		else
		{
			const QSize size = image.size();
			_layerSizeLabel->setText(QString("%1x%2").arg(QString::number(size.width())).arg(QString::number(size.height())));
		}
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
			int selectedIndex;
			if (_needAddFileToCombobox)
			{
				selectedIndex = _model.Add(path, image.size());
				_needAddFileToCombobox = false;
			}
			else
			{
				selectedIndex = _model.IndexOf(path);
			}

			_fileCombobox->setCurrentIndex(selectedIndex);

			for (int i = 0; i < layersCount; i++)
			{
				_layerCombobox->addItem(QString::number(i));
			}
			_layerCombobox->setCurrentIndex(0);

			_scene->setFixedSize(image.size());
			SetImage(image);
		}

		_layerCombobox->blockSignals(false);
		_fileCombobox->blockSignals(false);

		this->setEnabled(true);
	}

	void OnPyramidLayerChanged(const QPixmap& image)
	{
		SetImage(image);
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
