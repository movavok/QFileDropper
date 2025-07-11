#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

class DbManager : public QObject
{
    Q_OBJECT
public:
    explicit DbManager(QObject *parent = nullptr);
    ~DbManager();

    void saveField(const QString& field, const QString& value);
    void deleteField(const QString& field);
    bool isFieldSet(const QString &fieldName);
    QString loadField(const QString& field);

private:
    QSqlDatabase db;
    void init();
};

#endif // DBMANAGER_H
