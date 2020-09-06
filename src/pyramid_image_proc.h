#ifndef PYRAMID_IMAGE_PROC_H
#define PYRAMID_IMAGE_PROC_H

#include <QObject>

#include <memory>

class QPixmap;
class QString;

class PyramidImageProc : public QObject
{
	Q_OBJECT
public:
	PyramidImageProc();
	~PyramidImageProc();

	void BeginLoadingImage(const QString& path);
	void BeginChangeLayer(int newLayer);

signals:
	void ImageLoaded(const QString& path, const QPixmap& image, int layersCount);
	void LayerChanged(const QPixmap& image);

private:
	class Impl;
	Impl* const _impl;
};

#endif // PYRAMID_IMAGE_PROC_H
