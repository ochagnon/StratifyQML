#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QDir>

#include "Helper.h"
#include "PortIO.h"

using namespace StratifyIO;


QJsonObject PortIO::mPortSettings;
QList<PortIO> PortIO::mPortList;

void PortIO::init(){
    QString path;
    QFile settingsFile;

    path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + "PortIO.json";

    settingsFile.setFileName(path);

    qDebug() << Q_FUNC_INFO << "Load port settings from" << path;


    //load the settings
    if( settingsFile.open(QFile::ReadOnly) == true ){
        //read the data into mPortSettings JsonObject
        mPortSettings = QJsonDocument::fromJson( settingsFile.readAll() ).object();
        settingsFile.close();
    } else {
        qDebug() << Q_FUNC_INFO << "Failed to load port settings";
    }

}

void PortIO::exit(){

    //save the settings
    QFile settingsFile;
    QString path;
    QDir dir;

    dir.setPath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    path = dir.path() + "/" + "PortIO.json";
    settingsFile.setFileName(path);

    qDebug() << Q_FUNC_INFO << "Save port settings to" << path;

    if( dir.exists() == false ){
        dir.mkpath( dir.path() );
    }

    if( settingsFile.open(QFile::WriteOnly) == true ){
        QJsonDocument doc;
        doc.setObject(mPortSettings);
        settingsFile.write( doc.toJson() );
        settingsFile.close();
    }


}

bool PortIO::reconnect(Link & link, int count, int delay){
    int i;
    const PortIO * port;

    for(i=0; i < count; i++){

        QThread::msleep(delay/2);
        refreshPortList(link);

        //get the serial number of the last connected device in the new list
        port = lookupSerialNumber(link.serial_no().c_str());
        if( port != 0 ){
            qDebug() << "Reconnect" << link.serial_no().c_str() << "to" << port->linkSerialPortInfo().systemLocation();
            link.init(port->linkPath().toStdString(),
                      link.serial_no(),
                      port->notifyPath().toStdString());
        }

        QThread::msleep(delay/2);

        if ( link.get_is_connected() ){
            return true;
        }

    }

    return false;
}


const PortIO * PortIO::lookupSerialNumber(const QString & serialNumber){
    for(int i=0; i < portList().count(); i++){
        if( portList().at(i).linkSerialPortInfo().serialNumber() == serialNumber ){
            return &(portList().at(i));
        }
    }
    return 0;
}


void PortIO::refreshPortList(Link & link){
    //use QSerialPort to get a list of known devices

    if( link.get_is_connected() ){
        link.exit();
    }

    QList<QSerialPortInfo> list;
    list = QSerialPortInfo::availablePorts();
    int i;
    int j;

    qDebug() << "There are" << list.count() << "ports available";

    mPortList.clear();
    i = 0;

    i = 0;
    foreach(QSerialPortInfo info, list){
        bool alreadyAdded = false;

        qDebug() << "Device:" << i++;
        qDebug() << "\tMfg" << info.manufacturer();
        qDebug() << "\tPID" << info.productIdentifier();
        qDebug() << "\tVID" << info.vendorIdentifier();
        qDebug() << "\tName" << info.portName();
        qDebug() << "\tSystem Name" << info.systemLocation();
        qDebug() << "\tDescription" << info.description();
        qDebug() << "\tSerial Number" << info.serialNumber();


#if defined Q_OS_OSX
        //is the device a StratifyOS device
        if( info.description() == "StratifyOS" && info.portName().startsWith("cu.") ){  //description on Mac OS X is "StratifyOS"
            //is the serial number already accounted for
            for(j=0; j < mPortList.count(); j++){
                PortIO & item = mPortList[j];
                if( item.linkSerialPortInfo().serialNumber() == info.serialNumber() ){
                    alreadyAdded = true;
                    if( item.linkSerialPortInfo().systemLocation() != info.systemLocation() ){
                        qDebug() << info.serialNumber() <<
                                    "already added -- add notify port" << info.systemLocation();
                        item.mNotifySerialPortInfo = info;
                        item.mIsNotifyPortValid = true;
                    }
                }
            }

            if( !alreadyAdded ){
                //load sys attr
                sys_attr_t attr;

                if( getSysAttrCache(info.serialNumber(), attr) == false ){
                    qDebug() << Q_FUNC_INFO << info.serialNumber() << "not in cache";
                    if( loadSysAttr(link, info.systemLocation(), attr) == 0 ){
                        if( QString(attr.name) != "bootloader" ){
                            setSysAttrCache(info.serialNumber(), attr);
                        }
                    }
                } else {
                    qDebug() << Q_FUNC_INFO << "Loaded cached sysAttr";
                }

                qDebug() << "Add" << QString(attr.name) << "on" << info.systemLocation();
                PortIO item(info, attr);
                if( QString(attr.name) == "bootloader" ){
                    item.mIsBootloader = true;
                } else {
                    item.mIsBootloader = false;
                }
                mPortList.append(item);


            }
        }
#endif

#if defined Q_OS_WIN

        if( (info.description() == "StratifyOS Link Port") ){

            for(i=0; i < mPortList.count(); i++){
                PortIO & item = mPortList[i];
                if( item.linkSerialPortInfo().serialNumber() == info.serialNumber() ){
                    alreadyAdded = true;
                }
            }

            if( !alreadyAdded ){
                sys_attr_t attr;
                qDebug() << "Load Sys Attr";
                if( loadSysAttr(link, info.systemLocation(), attr) == 0 ){
                    PortIO item(info, attr);
                    if( QString(attr.name) == "bootloader" ){
                        item.mIsBootloader = true;
                    } else {
                        item.mIsBootloader = false;
                    }


                    foreach(QSerialPortInfo notify, list){
                        if( (notify.description() == "StratifyOS Notify Port") &&
                                (notify.serialNumber() == info.serialNumber()) ){
                            item.mIsNotifyPortValid = true;
                            item.mNotifySerialPortInfo = notify;
                        }
                    }

                    mPortList.append(item);
                    qDebug() << "Add" << QString(attr.name) << "on" << info.systemLocation();

                }
            }
        }
#endif
    }
}

void PortIO::setSysAttrCache(const QString & serialNumber, sys_attr_t & attr){
    qDebug() << Q_FUNC_INFO << "Add" << attr.version << attr.sys_version << attr.name << "to Cache";
    mPortSettings.insert(serialNumber, Helper::dataToJson("sysAttr", &attr, sizeof(sys_attr_t)));
}

bool PortIO::getSysAttrCache(const QString & serialNumber, sys_attr_t & attr){
    QStringList keys = mPortSettings.keys();
    foreach(QString key, keys){
        qDebug() << Q_FUNC_INFO << "Compare" << key << "From cache to" << serialNumber;
        if( key == serialNumber ){
            qDebug() << Q_FUNC_INFO << "Found a match in the cache";
            if( Helper::dataFromJson(mPortSettings.value(key).toObject(), "sysAttr", &attr, sizeof(sys_attr_t)) == true ){
                return true;
            } else {
                qDebug() << "Failed to load data from cache";
            }
        }
    }
    return false;
}

int PortIO::loadSysAttr(Link & link, const QString & systemLocation, sys_attr_t & attr){
    memset(&attr, 0, sizeof(sys_attr_t));
    int check_bootloader;
    //check the local database to see if the info is already available


    link.driver()->dev.handle = link.driver()->dev.open(systemLocation.toStdString().c_str(), 0);
    if( link.driver()->dev.handle != LINK_PHY_OPEN_ERROR ){

        qDebug() << "Check bootloader";
        check_bootloader = link_isbootloader(link.driver());
        if( check_bootloader > 0 ){
            strcpy(attr.name, "bootloader");
        } else if( check_bootloader == 0 ){
            int sysFd;
            qDebug() << "Open /dev/sys";
            sysFd = link_open(link.driver(), "/dev/sys", LINK_O_RDWR);
            if( sysFd >= 0 ){
                if( link_ioctl(link.driver(), sysFd, I_SYS_GETATTR, &attr) == 0 ){
                    //now add the sys_attr to the string list
                    attr.name[NAME_MAX-1] = 0;  //make sure these are zero terminated
                    attr.version[7] = 0;
                }
                link_close(link.driver(), sysFd);
            } else {
                qDebug() << "Failed to open /dev/sys";
            }
        } else if( check_bootloader < 0 ){
            qDebug() << "Failed to check bootloader";
            strcpy(attr.name,"error");
        }
        qDebug() << "Close";
        link.driver()->dev.close(&(link.driver()->dev.handle));
        qDebug() << "Close complete";
    } else {
        qDebug() << "Failed to open" << systemLocation;
        return -1;
    }
    return 0;

}
