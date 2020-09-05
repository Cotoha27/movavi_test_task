#include "pyramid_image_proc.h"

#include <QMap>
#include <QMutex>
#include <QPixmap>
#include <QThreadPool>

namespace {

inline int GetLayerCount(const QSize& size)
{
	if (size.isEmpty())
		return -1;

	int layerCount = 0;

	int w = size.width();
	int h = size.height();
	while (w > 1 && h > 1)
	{
		w /= 2;
		h /= 2;

		if (w < 1)
			w = 1;

		if (h < 1)
			h = 1;

		layerCount++;
	}

	return layerCount;
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
				emit _parent->ProcessingCompleted();
			}
			else
			{
				_currentInfo = nullptr;
				_currentPixmap = nullptr;

				auto task = [this, path]()
				{
					QPixmap pixmap(path);

					if (pixmap.isNull())
					{
						emit this->LoadTaskCompleted(QString(), QPixmap(), -1);
					}
					else
					{
						const int layerCount = GetLayerCount(pixmap.size());
						emit this->LoadTaskCompleted(path, std::move(pixmap), layerCount);
					}
				};

				QThreadPool::globalInstance()->start(task);
			}
		}
	}

	void SetCurrentLayer(int layer)
	{
		if (_currentInfo == nullptr)
			return;

		if (_currentInfo->cachedLayers.contains(layer))
		{
			_currentPixmap = &_currentInfo->cachedLayers[layer];
		}
		else
		{
			_currentPixmap = nullptr;

			// todo: run async generate layer
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
	void LoadTaskCompleted(const QString& path, QPixmap pixmap, int layerCount);

private slots:
	void OnLoadTaskCompleted(const QString& path, QPixmap pixmap, int layerCount)
	{
		if (!pixmap.isNull())
		{
			_cachedImages.insert(path, { layerCount, {{0, pixmap}} });
			_currentPixmap = &pixmap;
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