#include "skyView.h"

#include <QPainter>
#include <math.h>

#define RM		20
#define IDIAM    5       /* satellite icon diameter */

SkyView::SkyView(QWidget *parent) : QWidget(parent)
{
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);
}


static void draw_arc(QPainter &painter, int x, int y, unsigned int diam)
{
	painter.drawArc(x - diam / 2, y - diam / 2,        /* x,y */
					diam, diam,        /* width, height */
					0, 360 * 64);      /* angle1, angle2 */
}

static int min(int x, int y)
{
	return x < y ? x : y;
}

static void pol2cart(int width, int height, double azimuth, double elevation,  int *xout, int *yout)
{
	int diameter = min(width, height) - RM;
	azimuth *= DEG_2_RAD;
	elevation = sin((90.0 - elevation) * DEG_2_RAD);
	*xout = (int)((width / 2) + sin(azimuth) * elevation * (diameter / 2));
	*yout = (int)((height / 2) - cos(azimuth) * elevation * (diameter / 2));
}

void SkyView::paintEvent(QPaintEvent *)
{
	// Draw stuff in drawing area
	int w= width();
	int h= height();
	QPainter painter(this);
	painter.setPen(Qt::gray);
	QRect r1(0, 0, w-1, h-1);
	QBrush br1(Qt::white);
	painter.fillRect(r1, br1);

	draw_arc(painter, w/2, h / 2, 6);

	// draw the 45 degree circle
	int i = min(w, h);
	draw_arc(painter, w / 2, h / 2, (unsigned)((i - RM) * 0.7));

	painter.setPen(Qt::black);
	draw_arc(painter, w / 2, h / 2, (unsigned)(i - RM));
	
	int x, y;
	pol2cart(w, h, 0, 0, &x, &y);
	painter.drawText(x, y, "N");
	pol2cart(w, h, 90, 0, &x, &y);
	painter.drawText(x + 2, y, "E");
	pol2cart(w, h, 180, 0, &x, &y);
	painter.drawText(x, y + 10, "S");
	pol2cart(w, h, 270, 0, &x, &y);
	painter.drawText(x - 5, y, "W");

	if(gpsdata == NULL)
		return;
	
	/* Now draw the satellites... */
	for (int i = 0; i < gpsdata->satellites; i++) {
		QColor col;
		pol2cart(w, h, (double)gpsdata->azimuth[i], 
				 (double)gpsdata->elevation[i], &x, &y);
		if (gpsdata->ss[i] < 10) 
			col= Qt::black;
		else if (gpsdata->ss[i] < 30)
			col= Qt::red;
		else if (gpsdata->ss[i] < 35)
			col= Qt::yellow;
		else if (gpsdata->ss[i] < 40)
			col= Qt::darkGreen;
		else
			col= Qt::green;

		painter.setPen(col);
		br1.setColor(col);
		painter.setBrush(br1);

		if (gpsdata->PRN[i] > GPS_PRNMAX) {
				/* SBAS satellites */
			QPoint vertices[5];

			vertices[0].setX(x);
			vertices[0].setY(y-IDIAM);
			vertices[1].setX(x+IDIAM);
			vertices[1].setY(y);
			vertices[2].setX(x);
			vertices[2].setY(y+IDIAM);
			vertices[3].setX(x-IDIAM);
			vertices[3].setY(y);
			vertices[4].setX(x);
			vertices[4].setY(y-IDIAM);

	
			if (gpsdata->used[i])
				painter.drawPolygon(vertices, 4);
			else
				painter.drawPolyline(vertices, 5);
	
		} else {
				/* ordinary GPS satellites */
			if (gpsdata->used[i])
				painter.drawChord(x - IDIAM, y - IDIAM, 2 * IDIAM + 1, 2 * IDIAM + 1, 0, 360 * 64);
			else
				painter.drawArc(x - IDIAM, y - IDIAM, 2 * IDIAM + 1, 2 * IDIAM + 1, 0, 360 * 64);
		}
		QString buf;
		buf.setNum(gpsdata->PRN[i]);
		painter.setPen(Qt::black);
		painter.drawText(x, y+17, buf);
	}
}
