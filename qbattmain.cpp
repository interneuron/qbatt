#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <QDebug>
#include "qbattmain.h"
#include "qbattstats.h"

QBattMain::QBattMain(QWidget *parent) :
	QMainWindow(parent)
{
	trayFont = QFont("Hack", 10);
	trayPixmap = QPixmap(16, 16);
	trayPixmap.fill(Qt::white);

	trayIcon = new QBattTray_T (this);

	stats = new QBattStats();

	trayTimer = new QTimer(this);
	trayTimer->setInterval(3000);
	trayTimer->start();

	QObject::connect(trayTimer, SIGNAL(timeout()),
					 this, SLOT(updateTrayLabel()));

  QObject::connect(trayIcon->getSystemTray (), SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
           this, SLOT(exitApplication(QSystemTrayIcon::ActivationReason)));

}

QBattMain::~QBattMain()
{
	if (trayTimer->isActive())
		trayTimer->stop();

	// Release memory
	delete stats;
	delete trayTimer;
	delete trayIcon;
}

void QBattMain::updateTrayLabel()
{
	// Update PSU information first
	int currentRate = -1;
	float battery_health = 0.0;
	stats->updatePowerSupplyInfo();
	int trayCapacity = stats->getBatteryCapacity();
	trayIcon->batteryLevelChanged(trayCapacity);
	QString battStatus = stats->getBatteryStatus();
	bool adapterStatus = stats->getACOnline();
	trayIcon->batteryStatusChanged (adapterStatus);

	currentRate = stats->getBatteryCurrentNow();
	if (currentRate != -1)
		currentRate /= 1000;
	else
		currentRate = (stats->getBatteryPowerNow() / (stats->getBatteryVoltageNow() / 1000));

	trayToolTipText.clear();

	trayToolTipText.append("Status: ");
	if ((!QString().compare(battStatus, BATT_STATUS_UNKNOWN)) and
			(adapterStatus)) {
		trayToolTipText.append("On-line");
	} else {
		trayToolTipText.append(battStatus);
	}

	trayText.clear();

	if (trayCapacity >= 100) {
		/*
		 * Capacity value might be greater than 100% when
		 * the battery has accumulated greater energy amount
		 * at its last charge in compare to its previous charge.
		 * So just show that it's completely charged.
		 */
		trayText.append("F");
	} else {
		if ((!QString().compare(battStatus, BATT_STATUS_CHARGING)) or
				(!QString().compare(battStatus, BATT_STATUS_DISCHARGING))) {
			trayToolTipText.append(QString().sprintf("\nRate: %d mAh",
													 currentRate));
			trayToolTipText.append("\nTime left: ");
			trayToolTipText.append(stats->getTimeLeft());
		}

		trayText.sprintf("%d", trayCapacity);
	}

	trayToolTipText.append(QString().sprintf("\nCapacity: %d%%", trayCapacity));

	if ((stats->getBatteryChargeFull() != 0) &&
		(stats->getBatteryChargeFullDesign() != 0)) {
		battery_health = (stats->getBatteryChargeFull() * 100.0) /
						stats->getBatteryChargeFullDesign() * 1.0;
		trayToolTipText.append(QString().sprintf("\nHealth: %.1f%%",
						battery_health));
	}

	trayPainter = new QPainter(&trayPixmap);
	trayPainter->setFont(trayFont);

	trayPainter->eraseRect(trayPixmap.rect());
	trayPainter->drawText(trayPixmap.rect(), Qt::AlignCenter, trayText);

	trayIcon->getSystemTray ()->setToolTip(trayToolTipText);

	delete trayPainter;
}

void QBattMain::exitApplication(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::DoubleClick) {
		QMessageBox *msg = new QMessageBox(QMessageBox::Information,
										   "Exit qbatt:", "Do you really want to exit?",
										   QMessageBox::Yes | QMessageBox::Cancel, NULL);

		int ret = msg->exec();
		delete msg;

		switch (ret) {
		case QMessageBox::Yes:
			// Stop the timer
			trayTimer->stop();
			// Release memory
			delete stats;
			delete trayTimer;
			delete trayIcon;

			exit(0);
			break;
		case QMessageBox::Cancel:
			break;
		}
	}

	return;
}
