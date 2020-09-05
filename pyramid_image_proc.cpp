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

	int layerCount = 0;

	while (size.width() > 1 && size.height() > 1)
	{
		size /= 2;

		if (size.width() < 1)
			size.setWidth(1);

		if (size.height() < 1)
			size.setHeight(1);

		layerCount++;
	}

	return layerCount;
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
	Q_OBJECT
private: // types
	struct ImageInfo
	{
		int layerCount;
		QMap<int, QPixmap> cachedLayers;
	};

private: // fields
	PyramidImageProc* const _parent;

	QMap<QString, ImageInfo> _cachedImages;

	ImageInfo* _currentInfo = nullptr;
	QPixmap* _currentPixmap = nullptr;

public:
	Impl(PyramidImageProc* parent)
		: _parent(parent)
	{
		connect(this, &Impl::LoadTaskCompleted, this, &Impl::OnLoadTaskCompleted, Qt::QueuedConnection);
		connect(this, &Impl::GenerateLayerTaskCompleted, this, &Impl::OnGenerateLayerTaskCompleted, Qt::QueuedConnection);
	}

public: // methods
	void SetCurrentImage(const QString& path)
	{
		if (path.isEmpty())
		{
			_currentInfo = nullptr;
			_currentPixmap = nullptr;
			emit _parent->ProcessingCompleted();
		}
		else
		{
			if (_cachedImages.contains(path))
			{
				_currentInfo = &_cachedImages[path];
				_currentPixmap = &_cachedImages[path].cachedLayers[0];
				emit _parent->ProcessingCompleted();
			}
			else
			{
				_currentInfo = nullptr;
				_currentPixmap = nullptr;

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
						const int layerCount = GetLayerCount(pixmap.size());
						emit emittedFunc(path, std::move(pixmap), layerCount);
					}
				};

				QThreadPool::globalInstance()->start(task);
			}
		}
	}

	void SetCurrentLayer(int layer)
	{
		if (_currentInfo == nullptr || _currentInfo->layerCount < layer)
			return;

		if (_currentInfo->cachedLayers.contains(layer))
		{
			_currentPixmap = &_currentInfo->cachedLayers[layer];
		}
		else
		{
			_currentPixmap = nullptr;

			using namespace std::placeholders;
			auto emittedFunc = std::bind(&Impl::GenerateLayerTaskCompleted, this, _1, _2);

			QPixmap* baseImage = &_currentInfo->cachedLayers[0];

			auto task = [emittedFunc, layer, baseImage]()
			{
				QSize newSize = GetLayerSize(baseImage->size(), layer);
				auto image = baseImage->scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

				if (image.isNull())
				{
					emit emittedFunc(QPixmap(), -1);
				}
				else
				{
					emit emittedFunc(std::move(image), layer);
				}
			};

			QThreadPool::globalInstance()->start(task);
		}
	}

	int GetCurrentLayerCount() const
	{
		return _currentInfo != nullptr ? _currentInfo->layerCount : -1;
	}

	const QPixmap* GetCurrentPixmap() const
	{
		return _currentPixmap;
	}

signals:
	void LoadTaskCompleted(const QString& path, QPixmap image, int layerCount);
	void GenerateLayerTaskCompleted(QPixmap pixmap, int layer);

private slots:
	void OnLoadTaskCompleted(const QString& path, QPixmap image, int layerCount)
	{
		if (!image.isNull())
		{
			_cachedImages.insert(path, { layerCount, {{0, image}} });
			_currentPixmap = &image;
		}

		emit _parent->ProcessingCompleted();
	}

	void OnGenerateLayerTaskCompleted(QPixmap image, int layer)
	{
		if (!image.isNull())
		{
			_currentInfo->cachedLayers.insert(layer, std::move(image));
			_currentPixmap = & _currentInfo->cachedLayers[layer];
		}

		emit _parent->ProcessingCompleted();
	}
};

PyramidImageProc::PyramidImageProc()
	: _impl(new Impl(this))
{ }

PyramidImageProc::~PyramidImageProc()
{
	delete _impl;
}

void PyramidImageProc::SetCurrentImage(const QString& path)
{
	_impl->SetCurrentImage(path);
}

void PyramidImageProc::SetCurrentLayer(int layer)
{
	_impl->SetCurrentLayer(layer);
}

int PyramidImageProc::GetCurrentLayerCount() const
{
	return _impl->GetCurrentLayerCount();
}

const QPixmap* PyramidImageProc::GetCurrentPixmap() const
{
	return _impl->GetCurrentPixmap();
}

#include "pyramid_image_proc.moc" // WTF? for avoid build error "No Q_OBJECT in the class with the signal"