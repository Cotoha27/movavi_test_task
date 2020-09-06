#include "pyramid_image_proc.h"

#include <QMap>
#include <QMutex>
#include <QPixmap>
#include <QThreadPool>

namespace {

inline int GetLayerCount(QSize size)
{
	if (size.isEmpty())
		return -1;

	int layersCount = 0;

	while (size.width() > 1 && size.height() > 1)
	{
		size /= 2;

		if (size.width() < 1)
			size.setWidth(1);

		if (size.height() < 1)
			size.setHeight(1);

		layersCount++;
	}

	return layersCount;
}

inline QSize GetLayerSize(QSize size, int layer)
{
	if (GetLayerCount(size) < layer)
		return QSize();

	for (int i = 0; i < layer; i++)
	{
		size /= 2;

		if (size.width() < 1)
			size.setWidth(1);

		if (size.height() < 1)
			size.setHeight(1);
	}

	return size;
}

} // namespace

class PyramidImageProc::Impl : public QObject
{
	Q_OBJECT // it shouldn't be here
private: // types
	struct ImageInfo
	{
		int layersCount = 0;
		QMap<int, QPixmap> cachedLayers;
	};

private: // fields
	PyramidImageProc* const _parent;

	QMap<QString, ImageInfo> _cachedImages;

	ImageInfo* _currentInfo = nullptr;

public:
	Impl(PyramidImageProc* parent)
		: _parent(parent)
	{
		connect(this, &Impl::LoadTaskCompleted, this, &Impl::OnLoadTaskCompleted, Qt::QueuedConnection);
		connect(this, &Impl::GenerateLayerTaskCompleted, this, &Impl::OnGenerateLayerTaskCompleted, Qt::QueuedConnection);
	}

public: // methods
	void BeginLoadingImage(const QString& path)
	{
		if (path.isEmpty())
		{
			_currentInfo = nullptr;
			emit _parent->ImageLoaded(QString(), QPixmap(), -1);
		}
		else
		{
			if (_cachedImages.contains(path))
			{
				_currentInfo = &_cachedImages[path];
				emit _parent->ImageLoaded(path, _cachedImages[path].cachedLayers[0], _currentInfo->layersCount);
			}
			else
			{
				_currentInfo = nullptr;

				using namespace std::placeholders;
				auto emittedFunc = std::bind(&Impl::LoadTaskCompleted, this, _1, _2, _3);

				auto task = [emittedFunc, path]()
				{
					QPixmap pixmap(path);

					if (pixmap.isNull())
					{
						emit emittedFunc(QString(), QPixmap(), -1);
					}
					else
					{
						const int layersCount = GetLayerCount(pixmap.size());
						emit emittedFunc(path, std::move(pixmap), layersCount);
					}
				};

				QThreadPool::globalInstance()->start(task);
			}
		}
	}

	void BeginChangeLayer(int newLayer)
	{
		if (_currentInfo == nullptr || _currentInfo->layersCount < newLayer)
		{
			emit _parent->LayerChanged(QPixmap());
			return;
		}

		if (_currentInfo->cachedLayers.contains(newLayer))
		{
			emit _parent->LayerChanged(_currentInfo->cachedLayers[newLayer]);
		}
		else
		{
			using namespace std::placeholders;
			auto emittedFunc = std::bind(&Impl::GenerateLayerTaskCompleted, this, _1, _2);

			QPixmap* baseImage = &_currentInfo->cachedLayers[0];

			auto task = [emittedFunc, newLayer, baseImage]()
			{
				QSize newSize = GetLayerSize(baseImage->size(), newLayer);
				auto image = baseImage->scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

				if (image.isNull())
				{
					emit emittedFunc(QPixmap(), -1);
				}
				else
				{
					emit emittedFunc(std::move(image), newLayer);
				}
			};

			QThreadPool::globalInstance()->start(task);
		}
	}

signals:
	void LoadTaskCompleted(const QString& path, QPixmap image, int layersCount);
	void GenerateLayerTaskCompleted(QPixmap pixmap, int layer);

private slots:
	void OnLoadTaskCompleted(const QString& path, QPixmap image, int layersCount)
	{
		if (!image.isNull())
		{
			_currentInfo = &_cachedImages.insert(path, { layersCount, {{0, image}} }).value();
		}

		emit _parent->ImageLoaded(path, image, layersCount);
	}

	void OnGenerateLayerTaskCompleted(QPixmap image, int layer)
	{
		if (!image.isNull())
		{
			_currentInfo->cachedLayers.insert(layer, image);
		}

		emit _parent->LayerChanged(image);
	}
};

PyramidImageProc::PyramidImageProc()
	: _impl(new Impl(this))
{ }

PyramidImageProc::~PyramidImageProc()
{
	delete _impl;
}

void PyramidImageProc::BeginLoadingImage(const QString& path)
{
	_impl->BeginLoadingImage(path);
}

void PyramidImageProc::BeginChangeLayer(int newLayer)
{
	_impl->BeginChangeLayer(newLayer);
}

#include "pyramid_image_proc.moc" // WTF? for avoid build error "No Q_OBJECT in the class with the signal"