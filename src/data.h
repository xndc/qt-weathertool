#pragma once
#include <QtCore/QIODevice>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QTextCodec>
#include <QtCore/QByteArray>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QStringListModel>
#include <QtCore/QDateTime>
#include <QtXml/QDomDocument>

// QStringList ReadAirportData (QByteArray csvFile);

struct AirportNameEntry {
    QString icao; // ICAO airport code for EditRole, e.g. "LROP"
    QString full; // data for DisplayRole, e.g. "LROP/OTP: Henri Coanda International Airport, Bucharest, Romania"
};

class AirportNameModel : public QAbstractItemModel {
    // DisplayRole -> LROP/OTP: Henri Coanda International Airport, Bucharest, Romania
    // EditRole    -> LROP
    public:
        void readData (QByteArray csvFile);
        QModelIndex index (int row, int column, const QModelIndex &parent = QModelIndex()) const;
        QModelIndex parent (const QModelIndex &index) const;
        int rowCount (const QModelIndex &parent = QModelIndex()) const;
        int columnCount (const QModelIndex &parent = QModelIndex()) const;
        QVariant data (const QModelIndex &index, int role = Qt::DisplayRole) const;
    private:
        QList<AirportNameEntry> entries;
};

class ForecastModel : public QAbstractTableModel {
    public:
        void readData (QIODevice* device);
        int rowCount (const QModelIndex& parent = QModelIndex()) const;
        int columnCount (const QModelIndex& parent = QModelIndex()) const;
        QVariant data (const QModelIndex &index, int role = Qt::DisplayRole) const;
        QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    private:
        QDomDocument document;
};

class MetarModel : public QAbstractTableModel {
    public:
        void readData (QIODevice* device);
        int rowCount (const QModelIndex& parent = QModelIndex()) const;
        int columnCount (const QModelIndex& parent = QModelIndex()) const;
        QVariant data (const QModelIndex &index, int role = Qt::DisplayRole) const;
        QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    private:
        QDomDocument document;
};
