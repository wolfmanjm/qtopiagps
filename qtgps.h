#ifndef QTGPS_H
#define QTGPS_H

#include <QMutex>
#include <QTimer>

#include "ui_qtgps.h"
#include "skyView.h"
#include "libgpsmm.h"

class QtGps : public QWidget
{
	Q_OBJECT

	public:
		QtGps(QWidget *parent = 0, Qt::WFlags f = 0);
		virtual ~QtGps();
		
		void setGpsData(struct gps_data_t *gpsdata, char *message, size_t len= 0, int level= 0);

	public slots:
		void newGpsData(struct gps_data_t *gpsdata, char *message);
		void init();

	protected:
		void paintEvent(QPaintEvent *event);
		void showEvent(QShowEvent *);
		void hideEvent(QHideEvent *);
		void closeEvent (QCloseEvent *) ;
		
	private:
		Ui::MainWindow ui;
		QMutex mutex;

		SkyView *skyView;
		struct gps_data_t gpsdata;
		gpsmm gps_rec;

		QString sentence;
		time_t timer;	/* time of last state change */
		int state;	/* or MODE_NO_FIX=1, MODE_2D=2, MODE_3D=3 */
		bool hidden;
};

// Singleton for sending signal from a callback
class ExtSig : public QObject
{
	Q_OBJECT
	public:
		static ExtSig *self();
		void send(struct gps_data_t* p, char* buf);

	signals:
		void sendit(struct gps_data_t* p, char* buf);

	private:
		ExtSig();
};
#endif
