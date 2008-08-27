#ifndef SKYVIEW_H
#define SKYVIEW_H

#include <QObject>
#include <QWidget>

#include <gps.h>

class SkyView : public QWidget
{
	Q_OBJECT

	public:
		SkyView(QWidget *parent = 0);
		void setGpsData(struct gps_data_t *gd){ gpsdata= gd; }
		
	protected:
		void paintEvent(QPaintEvent *event);

	private:
		struct gps_data_t *gpsdata;


};

#endif
