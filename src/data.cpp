#include "data.h"
#include <iostream>
using namespace std;

static QString processCSVField (QByteArray bytes) {
    QByteArray processed;
    for (auto b: bytes) {
        if (b != '"') processed += b;
    }
    if (processed == QByteArray("\\N")) {
        return QString();
    }
    return QString::fromUtf8(processed);
}



// AirportModel

void AirportNameModel::readData (QByteArray csvFile) {
    for (QByteArray line: csvFile.split('\n')) {
        if (line.isEmpty()) continue;

        // Example lines:
        // 1638,"Lisbon Portela Airport","Lisbon","Portugal","LIS","LPPT",38.7812995911,-9.13591957092,...
        // 1631,"Montijo Airport","Montijo","Portugal",\N,"LPMT",38.703899383499994,-9.035920143130001,...
        QList<QByteArray> fields = line.split(',');

        auto name    = processCSVField(fields[1]);
        auto city    = processCSVField(fields[2]);
        auto country = processCSVField(fields[3]);
        auto iata    = processCSVField(fields[4]);
        auto icao    = processCSVField(fields[5]);

        AirportNameEntry entry;
        entry.icao = QString(icao);
        if (city.isEmpty() && iata.isEmpty()) {
            entry.full = QString("%1: %2, %3").arg(icao, name, country);
        } else if (city.isEmpty()) {
            entry.full = QString("%1/%2: %3, %4").arg(icao, iata, name, country);
        } else if (iata.isEmpty()) {
            entry.full = QString("%1: %2, %3, %4").arg(icao, name, city, country);
        } else {
            entry.full = QString("%1/%2: %3, %4, %5").arg(icao, iata, name, city, country);
        }

        entries.append(entry);
    }
}

QModelIndex AirportNameModel::index (int row, int column, const QModelIndex &parent) const {
    if (column != 0 || row < 0 || row >= rowCount())
        return QModelIndex();
    return createIndex(row, column);
}

QModelIndex AirportNameModel::parent (const QModelIndex &index) const {
    return QModelIndex();
}

int AirportNameModel::rowCount (const QModelIndex &parent) const {
    return entries.length();
}

int AirportNameModel::columnCount (const QModelIndex &parent) const {
    return 1;
}

QVariant AirportNameModel::data (const QModelIndex &index, int role) const {
    if (index.column() != 0 || index.row() < 0 || index.row() >= rowCount())
        return QVariant();
    if (role == Qt::DisplayRole)
        return QVariant(entries[index.row()].full);
    if (role == Qt::EditRole)
        return QVariant(entries[index.row()].icao);
    return QVariant();
}



// ForecastModel

void ForecastModel::readData (QIODevice* device) {
    document.setContent(device);
    endResetModel();
}

int ForecastModel::rowCount (const QModelIndex& parent) const {
    return document.elementsByTagName("forecast").count();
}

int ForecastModel::columnCount (const QModelIndex& parent) const {
    return 6;
}

QVariant ForecastModel::data (const QModelIndex &index, int role) const {
    if (role != Qt::DisplayRole) return QVariant();

    auto taf = document.elementsByTagName("TAF").item(0).toElement();
    auto f = taf.elementsByTagName("forecast").item(index.row()).toElement();
    if (f.isNull()) return QVariant();

    switch (index.column()) {
        case 0: return QVariant(f.elementsByTagName("fcst_time_from").item(0).toElement().text());
        case 1: return QVariant(f.elementsByTagName("fcst_time_to").item(0).toElement().text());
        case 2: return QVariant(f.elementsByTagName("change_indicator").item(0).toElement().text());
        case 3: return QVariant(f.elementsByTagName("wx_string").item(0).toElement().text());
        case 4: return QVariant(f.elementsByTagName("sky_condition").item(0).toElement().attribute("sky_cover"));
        case 5: return QVariant(taf.elementsByTagName("raw_text").item(0).toElement().text());
    }
    return QVariant();
}

QVariant ForecastModel::headerData (int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) return QVariant();
    if (orientation != Qt::Orientation::Horizontal) return QVariant();
    switch (section) {
        case 0: return QVariant(tr("From"));
        case 1: return QVariant(tr("To"));
        case 2: return QVariant(tr("Change indicator"));
        case 3: return QVariant(tr("Weather condition"));
        case 4: return QVariant(tr("Sky condition"));
        case 5: return QVariant(tr("Raw TAF text"));
    }
    return QVariant();
}



// MetarModel

void MetarModel::readData (QIODevice* device) {
    document.setContent(device);
    endResetModel();
}

int MetarModel::rowCount (const QModelIndex& parent) const {
    return document.elementsByTagName("METAR").count();
}

int MetarModel::columnCount (const QModelIndex& parent) const {
    return 8;
}

QVariant MetarModel::data (const QModelIndex &index, int role) const {
    if (role != Qt::DisplayRole) return QVariant();

    auto m = document.elementsByTagName("METAR").item(index.row()).toElement();
    if (m.isNull()) return QVariant();

    switch (index.column()) {
        case 0: return QVariant(m.elementsByTagName("observation_time").item(0).toElement().text());
        case 1: return QVariant(m.elementsByTagName("temp_c").item(0).toElement().text() + QString("°C"));
        case 2: return QVariant(m.elementsByTagName("dewpoint_c").item(0).toElement().text() + QString("°C"));
        case 3: return QVariant(m.elementsByTagName("wind_dir_degrees").item(0).toElement().text() + QString("°"));
        case 4: return QVariant(m.elementsByTagName("wind_speed_kt").item(0).toElement().text() + QString(" kt"));
        case 5: return QVariant(m.elementsByTagName("visibility_statute_mi").item(0).toElement().text() + QString("mi"));
        case 6: return QVariant(m.elementsByTagName("sky_condition").item(0).toElement().attribute("sky_cover"));
        case 7: return QVariant(m.elementsByTagName("raw_text").item(0).toElement().text());
    }
    return QVariant();
}

QVariant MetarModel::headerData (int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) return QVariant();
    if (orientation != Qt::Orientation::Horizontal) return QVariant();
    switch (section) {
        case 0: return QVariant(tr("Time"));
        case 1: return QVariant(tr("Temperature"));
        case 2: return QVariant(tr("Dew point"));
        case 3: return QVariant(tr("Wind"));
        case 4: return QVariant(tr("Wind speed"));
        case 5: return QVariant(tr("Visibility"));
        case 6: return QVariant(tr("Sky condition"));
        case 7: return QVariant(tr("Raw METAR text"));
    }
    return QVariant();
}
