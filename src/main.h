#pragma once
#include <QtCore/QSettings>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStyle>
#include <QtWidgets/QProxyStyle>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QTableView>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QCompleter>
#include <QtXml/QDomDocument>
#include "data.h"

extern QSettings* gSettings;

// Workaround for https://bugreports.qt.io/browse/QTBUG-60018: Status bar rendered as inactive on macOS
class WORKAROUND_StatusBarStyle : public QProxyStyle {
    public:
        virtual void drawPrimitive (PrimitiveElement el, const QStyleOption *opt, QPainter *painter,
            const QWidget *widget=Q_NULLPTR) const
        {
            switch (el) {
                case QStyle::PE_PanelStatusBar:
                    baseStyle()->drawPrimitive(el, opt, painter, nullptr);
                    break;
                default:
                    baseStyle()->drawPrimitive(el, opt, painter, widget);
            }
        }
};

class MainWindow : public QMainWindow {
    Q_OBJECT
    public:
        MainWindow(QWidget *parent = nullptr);
        void getAirportData();
        void configureSearch(QByteArray airportDataCSV);
        void weatherRequestsFinished();
    public slots:
        void airportDataRequestProgress(qint64 bytesReceived, qint64 bytesTotal);
        void airportDataRequestFinished();
        void searchSubmitted();
        void completionActivated(const QString &text);
        void weatherTAFRequestFinished();
        void weatherMETARRequestFinished();
    private:
        WORKAROUND_StatusBarStyle* _WORKAROUND_StatusBarStyle;
        QProgressBar* progressBar;

        QVBoxLayout* mainLayout;
        QLineEdit* searchEdit;
        QCompleter* searchCompleter;
        QFrame* resultsFrame;
            QVBoxLayout* resultsLayout;
            QGroupBox* forecastGroupBox;
                QVBoxLayout* forecastLayout;
                QTableView* forecastTable;
            QGroupBox* metarGroupBox;
                QVBoxLayout* metarLayout;
                QTableView* metarTable;

        QNetworkAccessManager* networkAccessManager;

        QNetworkReply* airportDataReply;
        AirportNameModel* airportNameModel = nullptr;

        QNetworkReply* weatherTAFReply   = nullptr;
        QNetworkReply* weatherMETARReply = nullptr;
        QString weatherRequestsAirportCode;

        // Used from the weather*RequestFinished slots to check whether or not to run
        // weatherRequestsFinished. Inelegant, but will do for now.
        bool weatherTAFRequestDidFinish = false;
        bool weatherMETARRequestDidFinish = false;
        bool weatherRequestsDidFail = false;

        ForecastModel forecastModel;
        MetarModel metarModel;
};
