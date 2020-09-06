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

	void StartSetCurrentImage(const QString& path);
	void StartSetCurrentLayer(int layer);

	int GetCurrentLayerCount() const;

	const QPixmap* GetCurrentPixmap() const;

signals:
	void ImageLoaded(const QString& path);
	void LayerChanged();

private:
	class Impl;
	Impl* const _impl;
};

#endif // PYRAMID_IMAGE_PROC_H
