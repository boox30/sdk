
#include "device_conf.h"

#ifdef BUILD_FOR_ARM
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include "onyx/base/device.h"

namespace sys
{

bool DeviceConfig::makeSureTableExist(QSqlDatabase& database)
{
    QSqlQuery query(database);
    bool ok = query.exec("create table if not exists device_conf ("
                         "key text primary key, "
                         "value text "
                         ")");
    return ok;
}


QString DeviceConfig::serialNumber(QSqlDatabase& database)
{

    return serialNumberFromDatabase(database);
}

QString DeviceConfig::deviceId(QSqlDatabase& database)
{
    return deviceIdFromDatabase(database);
}

QString DeviceConfig::version()
{
    // Always use the version defined in application.
    static QString version;
    if (version.isEmpty())
    {
        version = qgetenv("VERSION");
    }
    return version;
}

QString DeviceConfig::deviceIdFromDatabase(QSqlDatabase &db)
{
    QString ret;
    QSqlQuery query(db);
    query.prepare( "select value from device_conf where key = ?");
    query.addBindValue("device_id");
    if (query.exec() && query.next())
    {
        ret = query.value(0).toString();
    }

    if (!ret.isEmpty())
    {
        return ret;
    }

    ret = QUuid::createUuid().toString();
    query.prepare( "INSERT OR REPLACE into device_conf (key, value) values(?, ?)");
    query.addBindValue("device_id");
    query.addBindValue(ret);
    query.exec();
    return ret;
}

QString DeviceConfig::serialNumberFromDatabase(QSqlDatabase& db)
{
    QString ret;
    QSqlQuery query(db);
    query.prepare( "select value from device_conf where key = ?");
    query.addBindValue("serial_number");
    if (query.exec() && query.next())
    {
        ret = query.value(0).toString();
    }

    if (!ret.isEmpty())
    {
        return ret;
    }

    ret = QUuid::createUuid().toString();
    query.prepare( "INSERT OR REPLACE into device_conf (key, value) values(?, ?)");
    query.addBindValue("serial_number");
    query.addBindValue(ret);
    query.exec();
    return ret;
}

}
