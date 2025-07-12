#include "dbmanager.h"

DbManager::DbManager(QObject *parent)
    : QObject{parent}
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("settings.db");
    if (!db.open()) {
        qDebug() << "Cannot open database:" << db.lastError().text();
    } else {
        init();
    }
}

DbManager::~DbManager() {
    if (db.isOpen()) db.close();
}

void DbManager::init() {
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS settings (field TEXT PRIMARY KEY, value TEXT)");
}

void DbManager::saveField(const QString& field, const QString& value) {
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO settings (field, value) VALUES (?, ?)");
    query.addBindValue(field);
    query.addBindValue(value);
    if (!query.exec()) qDebug() << "Failed to save" << field << ":" << query.lastError().text();
}

void DbManager::deleteField(const QString& field) {
    QSqlQuery query;
    query.prepare("DELETE FROM settings WHERE field = ?");
    query.addBindValue(field);
    if (!query.exec()) qDebug() << "Failed to delete" << field << ":" << query.lastError().text();
}

bool DbManager::isFieldSet(const QString &fieldName)
{
    QSqlQuery query;
    query.prepare("SELECT value FROM settings WHERE field = ?");
    query.addBindValue(fieldName);

    if (!query.exec()) {
        qDebug() << "Failed to read field:" << fieldName << "-" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        return !query.value(0).toString().isEmpty();
    }

    return false;
}

QString DbManager::loadField(const QString& field) {
    QSqlQuery query;
    query.prepare("SELECT value FROM settings WHERE field = ?");
    query.addBindValue(field);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}
