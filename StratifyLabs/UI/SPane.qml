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

import QtQuick 2.6
import "."

SItem {
    default property alias data: contents.data;

    implicitWidth: blockWidth ? ((parent != null) ? parent.width : 0) : contents.childrenRect.width + paddingHorizontal*2;
    implicitHeight: fillHeight ? ((parent != null) ? parent.height : 0) : contents.childrenRect.height + paddingVertical*2;
    blockWidth:  true;

    Flickable {
        id: contents;
        anchors.fill: parent;
        contentWidth: childrenRect.width;
        contentHeight: childrenRect.height;
    }

}
