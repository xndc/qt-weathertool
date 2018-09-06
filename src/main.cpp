#include "main.h"
#include "util.h"

#include <iostream>
using namespace std;

QSettings* gSettings = nullptr;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(qApp->applicationDisplayName());
    resize(900, 700);

    networkAccessManager = new QNetworkAccessManager();

    // Add status bar:
    setStatusBar(new QStatusBar(this));
    // Workaround for https://bugreports.qt.io/browse/QTBUG-60018: Status bar rendered as inactive on macOS
    _WORKAROUND_StatusBarStyle = new WORKAROUND_StatusBarStyle();
    statusBar()->setStyle(_WORKAROUND_StatusBarStyle);
    statusBar()->setSizeGripEnabled(true);

    // Add progress bar:
    progressBar = new QProgressBar();
    progressBar->hide();
    progressBar->setMaximumWidth(200);
    progressBar->setMinimum(0);
    statusBar()->addPermanentWidget(progressBar);

    // Add central widget with main layout:
    setCentralWidget(new QWidget(this));
    mainLayout = new QVBoxLayout();
    centralWidget()->setLayout(mainLayout);

        // Add search box:
        searchEdit = new QLineEdit();
        searchEdit->setEnabled(false);
        searchEdit->setPlaceholderText(tr("Type to search for an airport"));
        // searchEdit->setTextMargins(4, 4, 4, 4);
        mainLayout->addWidget(searchEdit);

        // Set up completion:
        searchCompleter = new QCompleter();
        searchCompleter->setCaseSensitivity(Qt::CaseInsensitive);
        searchCompleter->setFilterMode(Qt::MatchContains);
        searchCompleter->setCompletionRole(Qt::DisplayRole);
        searchEdit->setCompleter(searchCompleter);

        // Add results frame and layout (hidden by default):
        resultsFrame = new QFrame();
        resultsFrame->hide();
        // Setting stretch=1 makes this take up the maximum allowable space when visible, which is required because
        // we also add a stretch element to mainLayout to keep the search box on top when resultsFrame is invisible.
        mainLayout->addWidget(resultsFrame, 1);
        resultsLayout = new QVBoxLayout();
        resultsLayout->setContentsMargins(0, 0, 0, 0);
        resultsFrame->setLayout(resultsLayout);

            // Add forecast group box and layout:
            forecastGroupBox = new QGroupBox(tr("Forecast (TAF)"));
            resultsLayout->addWidget(forecastGroupBox, 1);
            forecastLayout = new QVBoxLayout();
            forecastGroupBox->setLayout(forecastLayout);
            forecastTable = new QTableView();
            forecastTable->setCornerButtonEnabled(false);
            forecastTable->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
            forecastLayout->addWidget(forecastTable);

            // Add METAR group box, layout and table view:
            metarGroupBox = new QGroupBox(tr("Weather reports (METAR)"));
            resultsLayout->addWidget(metarGroupBox, 2);
            metarLayout = new QVBoxLayout();
            metarGroupBox->setLayout(metarLayout);
            metarTable = new QTableView();
            metarTable->setCornerButtonEnabled(false);
            metarTable->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
            metarLayout->addWidget(metarTable);

        // Stretch the layout, to ensure the search box is on top:
        mainLayout->addStretch();

    // Hook up the search box and completer:
    connect(searchEdit,      &QLineEdit::returnPressed, this, &MainWindow::searchSubmitted);
    connect(searchCompleter, QOverload<const QString &>::of(&QCompleter::activated),
            this, &MainWindow::completionActivated);

    getAirportData();
}

void MainWindow::getAirportData() {
    // Display loading message and set progress bar to indefinite mode:
    statusBar()->showMessage(tr("Loading airport data..."));
    progressBar->show();
    progressBar->setMaximum(0);

    // Load the airport data if already downloaded:
    if (gSettings->contains("airportData")) {
        configureSearch(gSettings->value("airportData").toByteArray());
    } else {
        // Get the airport data file from OpenFlights:
        QNetworkRequest request;
        request.setUrl(QUrl("https://raw.githubusercontent.com/jpatokal/openflights/master/data/airports.dat"));
        airportDataReply = networkAccessManager->get(request);
        connect(airportDataReply, &QNetworkReply::downloadProgress, this, &MainWindow::airportDataRequestProgress);
        connect(airportDataReply, &QNetworkReply::finished,         this, &MainWindow::airportDataRequestFinished);
    }
}

void MainWindow::airportDataRequestProgress(qint64 bytesReceived, qint64 bytesTotal) {
    int progress = qRound(1000 * (double(bytesReceived) / double(bytesTotal)));
    progressBar->setMaximum(1000);
    progressBar->setValue(progress);
}

void MainWindow::airportDataRequestFinished() {
    progressBar->setMaximum(0); // indefinite

    // Check for errors:
    if (airportDataReply->error() != QNetworkReply::NoError) {
        progressBar->hide();
        statusBar()->showMessage(tr("Loading failed."));
        OpenMessageBox(this, QMessageBox::Icon::Warning, tr("Airport data download failed"),
            tr("Couldn't download airport data.\nQNetworkReply error code: %1")
            .arg(airportDataReply->error()));
    } else {
        configureSearch(airportDataReply->readAll());
    }

    airportDataReply->deleteLater();
    airportDataReply = nullptr;
}

void MainWindow::configureSearch(QByteArray airportDataCSV) {
    progressBar->hide();

    // Create an AirportNameModel and read the DAT file:
    if (airportNameModel != nullptr) {
        delete airportNameModel;
    }
    airportNameModel = new AirportNameModel();
    airportNameModel->readData(airportDataCSV);

    // Update the status bar:
    progressBar->hide();
    statusBar()->showMessage(tr("Ready"));

    // Enable completion:
    searchCompleter->setModel(airportNameModel);

    // Enable the search box:
    searchEdit->setEnabled(true);
    searchEdit->setFocus();

    // Save the data for a later launch if everything went OK:
    gSettings->setValue("airportData", QVariant(airportDataCSV));
}

void MainWindow::completionActivated(const QString& text) {
    // This can, e.g. when selecting a completion with the keyboard and pressing Enter, cause searchSubmitted
    // to be called twice, so requests are aborted and retried. Fortunately, this doesn't seem to be an issue.
    searchEdit->returnPressed();
}

void MainWindow::searchSubmitted() {
    // We basically reimplement what QCompleter does internally, because completions are not updated on selection.
    // e.g. if you type "Bucharest" and click on the entry for LROP, the completion will get stuck on LRBS.
    QString searchText = searchEdit->text();
    if (searchText.isEmpty()) return;
    auto results = airportNameModel->match(airportNameModel->index(0,0), Qt::DisplayRole, QVariant(searchText), 1,
        Qt::MatchContains);
    if (results.length() <= 0) return;
    QString airportCode = airportNameModel->data(results.first(), Qt::EditRole).toString();
    QString airportText = airportNameModel->data(results.first(), Qt::DisplayRole).toString();

    // Write this into the search bar, so the user can see what airport they picked:
    searchEdit->setText(airportText);
    searchEdit->selectAll();

    // Do we have a pending network request? If yes, cancel it.
    if (weatherTAFReply != nullptr) {
        weatherTAFReply->abort();
        weatherTAFReply->deleteLater();
    }
    if (weatherMETARReply != nullptr) {
        weatherMETARReply->abort();
        weatherMETARReply->deleteLater();
    }

    // Start network requests:
    weatherRequestsAirportCode = airportCode;
    weatherTAFRequestDidFinish = false;
    weatherMETARRequestDidFinish = false;
    weatherRequestsDidFail = false;

    statusBar()->showMessage(tr("Retrieving data for %1...").arg(airportCode));
    progressBar->setMaximum(0); // indefinite
    progressBar->show();

    QNetworkRequest tafRequest;
    QNetworkRequest metarRequest;
    tafRequest.setUrl(QUrl(QString(
        "https://aviationweather.gov/adds/dataserver_current/httpparam?dataSource=tafs&requestType=retrieve"
        "&format=xml&stationString=%1&hoursBeforeNow=24&mostRecent=true")
        .arg(airportCode)));
    metarRequest.setUrl(QUrl(QString(
        "https://aviationweather.gov/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve"
        "&format=xml&stationString=%1&hoursBeforeNow=48"
    ).arg(airportCode)));
    weatherTAFReply = networkAccessManager->get(tafRequest);
    weatherMETARReply = networkAccessManager->get(metarRequest);
    connect(weatherTAFReply,   &QNetworkReply::finished, this, &MainWindow::weatherTAFRequestFinished);
    connect(weatherMETARReply, &QNetworkReply::finished, this, &MainWindow::weatherMETARRequestFinished);
}

void MainWindow::weatherTAFRequestFinished() {
    if (weatherTAFReply->error() != QNetworkReply::NoError) {
        weatherRequestsDidFail = true;
        cerr << "weatherTAFRequestFinished: failed with code " << weatherMETARReply->error() << endl;
    }
    weatherTAFRequestDidFinish = true;
    if (weatherMETARRequestDidFinish) {
        weatherRequestsFinished();
    }
}

void MainWindow::weatherMETARRequestFinished() {
    if (weatherMETARReply->error() != QNetworkReply::NoError) {
        weatherRequestsDidFail = true;
        cerr << "weatherMETARRequestFinished: failed with code " << weatherMETARReply->error() << endl;
    }
    weatherMETARRequestDidFinish = true;
    if (weatherTAFRequestDidFinish) {
        weatherRequestsFinished();
    }
}

void MainWindow::weatherRequestsFinished() {
    progressBar->hide();
    if (weatherRequestsDidFail) {
        statusBar()->showMessage(tr("Failed to retrieve weather data for %1.").arg(weatherRequestsAirportCode));
    } else {
        statusBar()->showMessage(tr("Processing weather data..."));

        forecastModel.readData(weatherTAFReply);
        forecastTable->setModel(&forecastModel);
        forecastTable->horizontalHeader()->setMinimumSectionSize(50);
        forecastTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);

        metarModel.readData(weatherMETARReply);
        metarTable->setModel(&metarModel);
        metarTable->horizontalHeader()->setMinimumSectionSize(50);
        metarTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);

        statusBar()->showMessage(tr("Weather data loaded for %1.").arg(weatherRequestsAirportCode));
        resultsFrame->show();
    }

    weatherTAFReply->deleteLater();
    weatherMETARReply->deleteLater();
    weatherTAFReply = nullptr;
    weatherMETARReply = nullptr;
}

int main(int argc, char *argv[]) {
    // NOTE:
    // Qt doesn't ship OpenSSL. On Linux and macOS this isn't an issue, since they have dynamic OpenSSL libs
    // that Qt can use, but on Windows the DLLs have to be placed in the application directory manually.
    // Qt 5.11.1 currently requires OpenSSL 1.0.2. A Windows/MSVC build of 1.0.2o can be downloaded from
    // https://indy.fulgan.com/SSL/ (or see https://wiki.openssl.org/index.php/Binaries for other sources).
    // Copy ssleay32.dll (libssl) and libeay32.dll (libcrypto) to the EXE's directory and it should work.
    // Not doing this will result in an error (code 99) from QNetworkAccessManager.
    qDebug() << "Supports SSL:  " << QSslSocket::supportsSsl()
        << "\nLib Version Number: " << QSslSocket::sslLibraryVersionNumber()
        << "\nLib Version String: " << QSslSocket::sslLibraryVersionString()
        << "\nLib Build Version Number: " << QSslSocket::sslLibraryBuildVersionNumber()
        << "\nLib Build Version String: " << QSslSocket::sslLibraryBuildVersionString();

    QApplication app (argc, argv);
    QApplication::setOrganizationName("xndc");
    QApplication::setOrganizationDomain("io.github.xndc");
    QApplication::setApplicationName("WeatherTool");
    QApplication::setApplicationDisplayName("WeatherTool");

    // Don't pollute the system, store settings (just the downloaded airport data for now) locally:
    gSettings = new QSettings("qsettings.ini", QSettings::IniFormat);

    MainWindow wndMain;
    wndMain.show();
    return app.exec();
}
