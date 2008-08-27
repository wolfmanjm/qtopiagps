#include <QtGlobal>

#ifdef Q_WS_QWS
#include <QAction>
#include <QMenu>
#include <QtopiaApplication>
#else
#include <QApplication>
#endif

#include <QMessageBox>
#include <QtDebug>

#include "qtgps.h"
#include <math.h>

#include <iostream>
#include <string>
#include <unistd.h>


static enum deg_str_type deg_type = deg_dd;
struct unit_t {
	const char *legend;
	double factor;
};
static struct unit_t speedtable[] = {
	{ "knots",	MPS_TO_KNOTS },
	{ "mph",	MPS_TO_MPH },
	{ "kmh",	MPS_TO_KPH },
}, *speedunits = speedtable;
static struct unit_t alttable[] = {
	{ "feet",	METERS_TO_FEET },
	{ "meters",	1},
}, *altunits = alttable;


// Singleton defns for emiting a signal from a callback
ExtSig::ExtSig(){}
ExtSig *ExtSig::self()
{
	static ExtSig inst;
	return &inst;
}

void ExtSig::send(struct gps_data_t* p, char* buf)
{
	emit sendit(p, buf);
}

QtGps::QtGps(QWidget *parent, Qt::WFlags f) :  QWidget(parent, f)
{
	qDebug("In QtGps()");
#ifdef Q_WS_QWS
	setObjectName("Gps");
	QtopiaApplication::setInputMethodHint(this, QtopiaApplication::AlwaysOff);
	setWindowTitle(tr("Gps", "application header"));
#endif
				ui.setupUi(this);
	skyView= new SkyView();
	state= -1;
	timer= time(NULL);
	hidden= true;
	
	connect(ExtSig::self(), SIGNAL(sendit(struct gps_data_t *, char *)),
			this, SLOT(newGpsData(struct gps_data_t *, char *)), Qt::QueuedConnection);

	QTimer::singleShot(100, this, SLOT(init()));
}

QtGps::~QtGps()
{
	qDebug("In ~QtGps()");
	delete skyView;
}

static void callback(struct gps_data_t* p, char* buf, size_t /*len*/, int /*level*/) {
	if (p==NULL) {
		qWarning("Error polling gpsd");
		QMessageBox::critical(NULL, "ERROR", "Error polling gpsd");
		return;
	}

#if _NDEBUG
	qDebug() << buf;
	qDebug() << "Online:\t" << ((p->online != 0) ? "yes" : "no");
	qDebug() << "Status:\t" << p->status;
	qDebug() << "Mode:\t" << p->fix.mode;
	if (p->fix.mode>=MODE_2D) {
		qDebug() << "Longitude:\t" << p->fix.longitude;
		qDebug() << "Latitude:\t" << p->fix.latitude;
	}
	qDebug("============ ");
#endif

	// signal the GUI thread we have new gps data
	ExtSig::self()->send(p, buf);
}

void QtGps::init()
{
	//ui.satelliteList->clear();
	ui.satelliteList->setRowCount(MAXCHANNELS);

	// setup skyview widget for drawing into
	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->addWidget(skyView);
	ui.skyView->setLayout(vbox);

	bool found= false;
	//su = get_resource(toplevel, "speedunits", "kmh");
	const char *su= "mph";
	for (speedunits = speedtable; 
		 speedunits < speedtable + sizeof(speedtable)/sizeof(speedtable[0]);
		 speedunits++){
		if (strcmp(speedunits->legend, su)==0){
			found= true;
			break;
		}
	}
	if(!found){
		speedunits = speedtable;
		qWarning("xgps: unknown speed unit, defaulting to %s\n",
				speedunits->legend);
	}

	found= false;
	//au = get_resource(toplevel, "altunits", "meters");
	const char * au= "feet";
	for (altunits = alttable; 
		 altunits < alttable + sizeof(alttable)/sizeof(alttable[0]);
		 altunits++){
		if (strcmp(altunits->legend, au)==0){
			found= true;
			break;
		}
	}
	if(!found){
		altunits = alttable;
		qWarning("xgps: unknown altitude unit, defaulting to %s\n",
				altunits->legend);
	}

	// setup gps
	QString host;
	if(QApplication::arguments().size() > 1)
		host= QApplication::arguments().at(1);
	else
		host= "localhost";

	qDebug("Using host %s\n", (const char *)host.toAscii());

	struct gps_data_t *resp;

	resp= gps_rec.open(host.toAscii(), DEFAULT_GPSD_PORT);

	if (resp==NULL) {
		qWarning("error opening gpsd");
		QMessageBox::critical(NULL, "ERROR", "Error opening gpsd");
		QCoreApplication::quit();
		return;
	}

	if (gps_rec.set_callback(callback)!=0 ) {
		QMessageBox::critical(NULL, "ERROR", "Error setting callback");
		qWarning("Error setting callback");
		QCoreApplication::quit();
		return;
	}

#if 0
	qDebug("querying...");
	resp= gps_rec.query("w+o\n");
	qDebug("...queryed");
	setGpsData(resp, NULL);
#endif
}

void QtGps::setGpsData(struct gps_data_t *gd, char *buf, size_t, int)
{
	QMutexLocker locker(&mutex);
	gpsdata= *gd;
	
	sentence= buf == NULL ? "" : buf;
}

void QtGps::newGpsData(struct gps_data_t *gd, char *message)
{
	setGpsData(gd, message);
	if(!hidden)
		update();
}

void QtGps::paintEvent(QPaintEvent *)
{
	QMutexLocker locker(&mutex);
	
 	int newstate;
 	char s[128], *latlon;

	struct gps_data_t *gd= &gpsdata;
	
#ifndef Q_WS_QWS
	QString message(sentence);
	message.truncate(80);
	
	ui.statusLine->setText(message);
#endif
	
	skyView->setGpsData(gd);
	skyView->update();
	
	/* This is for the satellite status display */
	if (gd->satellites) {
		QString str;

		// PRN:   Elev:  Azim:  SNR:  Used:
		for (int i = 0; i < MAXCHANNELS; i++) {
			if (i < gd->satellites) {
				QTableWidgetItem *n = new QTableWidgetItem(str.setNum(gd->PRN[i]));
				n->setFlags(0);
				ui.satelliteList->setItem(i, 0, n);
				n = new QTableWidgetItem(str.setNum(gd->elevation[i]));
				n->setFlags(0);
				ui.satelliteList->setItem(i, 1, n);
				n = new QTableWidgetItem(str.setNum(gd->azimuth[i]));
				n->setFlags(0);
				ui.satelliteList->setItem(i, 2, n);
				n = new QTableWidgetItem(str.setNum(gd->ss[i]));
				n->setFlags(0);
				ui.satelliteList->setItem(i, 3, n);
				n = new QTableWidgetItem(gd->used[i] ? "Y" : "N");
				n->setFlags(0);
				ui.satelliteList->setItem(i, 4, n);
			}
		}
		ui.satelliteList->resizeColumnsToContents();
	}

	/* here are the value fields */
	if (isnan(gd->fix.time)==0) {
		(void)unix_to_iso8601(gd->fix.time, s, sizeof(s));
		ui.time->setText(s);
	} else
		ui.time->setText("n/a");

	if (gd->fix.mode >= MODE_2D) {
		latlon = deg_to_str(deg_type,
							fabs(gd->fix.latitude));
		(void)snprintf(s, sizeof(s), "%s %c", latlon,
					   (gd->fix.latitude < 0) ? 'S' : 'N');
		ui.latitude->setText(s);
	} else
		ui.latitude->setText("n/a");
	
	if (gd->fix.mode >= MODE_2D) {
		latlon = deg_to_str(deg_type,
							fabs(gd->fix.longitude));
		(void)snprintf(s, sizeof(s), "%s %c", latlon,
					   (gd->fix.longitude < 0) ? 'W' : 'E');
		ui.longitude->setText(s);
	} else
		ui.longitude->setText("n/a");

	if (gd->fix.mode == MODE_3D) {
		(void)snprintf(s, sizeof(s), "%f %s",
					   gd->fix.altitude * altunits->factor,
					   altunits->legend);
		ui.altitude->setText(s);
	} else
		ui.altitude->setText("n/a");

	if (gd->fix.mode >= MODE_2D && isnan(gd->fix.track)==0) {
		(void)snprintf(s, sizeof(s), "%f %s",
					   gd->fix.speed * speedunits->factor,
					   speedunits->legend);
		ui.speed->setText(s);
	} else
		ui.speed->setText("n/a");

	if (gd->fix.mode >= MODE_2D && isnan(gd->fix.track)==0) {
		(void)snprintf(s, sizeof(s), "%f degrees",
					   gd->fix.track);
		ui.track->setText(s);
	} else
		ui.track->setText("n/a");

	if (isnan(gd->fix.eph)==0) {
		(void)snprintf(s, sizeof(s), "%f %s",
					   gd->fix.eph * altunits->factor,
					   altunits->legend);
		ui.eph->setText(s);
	} else
		ui.eph->setText("n/a");

	if (isnan(gd->fix.epv)==0) {
		(void)snprintf(s, sizeof(s), "%f %s", 
					   gd->fix.epv * altunits->factor,
					   altunits->legend);
		ui.epv->setText(s);
	} else
		ui.epv->setText("n/a");

	if (gd->fix.mode == MODE_3D && isnan(gd->fix.climb)==0) {
		(void)snprintf(s, sizeof(s), "%f %s/sec", 
					   gd->fix.climb * altunits->factor,
					   altunits->legend);
		ui.climb->setText(s);
	} else
		ui.climb->setText("n/a");
	
// 	if (gd->set & DEVICEID_SET) {
// 		(void)strlcpy(s, "xgps: ", sizeof(s));
// 		(void)strlcpy(s+6, gd->gps_id, sizeof(s)-6);
// 		set_title(s);
// 	}

	if (gd->online == 0) {
		newstate = 0;
		(void)strcpy(s, "OFFLINE");
	} else {
		newstate = gd->fix.mode;

		switch (gd->fix.mode) {
			case MODE_2D:
				(void)snprintf(s, sizeof(s), "2D %sFIX",
							   (gd->status == STATUS_DGPS_FIX) ? "DIFF " :
							   "");
				break;
			case MODE_3D:
				(void)snprintf(s, sizeof(s), "3D %sFIX",
							   (gd->status == STATUS_DGPS_FIX) ? "DIFF " :
							   "");
				break;
			default:
				(void)strcpy(s, "NO FIX");
				break;
		}
	}
	if (newstate != state) {
		timer = time(NULL);
		state = newstate;
	}
	(void)snprintf(s + strlen(s), sizeof(s) - strlen(s), " (%d secs)",
				   (int) (time(NULL) - timer));
	ui.status->setText(s);
}

void QtGps::showEvent(QShowEvent *)
{
	qDebug("In show");
	hidden= false;
}

void QtGps::hideEvent(QHideEvent *)
{
	qDebug("In hide");
	hidden= true;
}

void QtGps::closeEvent (QCloseEvent *)
{
	qDebug("In close");

	// stop the callbacks
	if(gps_rec.del_callback()!=0) {
		qDebug("Error deleting callback");
	}
}
