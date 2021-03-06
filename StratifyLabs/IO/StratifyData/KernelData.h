/*
Copyright 2016 Tyler Gilbert

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef HARDWARE_H
#define HARDWARE_H


#include <QString>
#include "Data.h"
#include "DeviceData.h"

namespace StratifyData {

class KernelData : public Data
{
public:
    KernelData(DataService * service = 0);
    KernelData(const QJsonObject & object, DataService * service = 0);

    //keys unique to hardware
    static QString assetsKey(){ return "assets"; }
    static QString bootSuffixKey() { return "bootsuffix"; }
    static QString deviceListKey() { return "devicelist"; }

    void setAssets(const QJsonObject & assets);
    void setBootSuffix(const QString & value);

    QString bootSuffix() const;

    QStringList deviceList() const;
    //DeviceData device(const QString & serialNo);


};

}

#endif // HARDWARE_H
