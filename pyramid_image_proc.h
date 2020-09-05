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

	void SetCurrentImage(const QString& path);
	void SetCurrentLayer(int layer);

	int GetCurrentLayerCount() const;

	const QPixmap* GetCurrentPixmap() const;

signals:
	void ProcessingCompleted();

private:
	class Impl;
	Impl* const _impl;
};

#endif // PYRAMID_IMAGE_PROC_H
