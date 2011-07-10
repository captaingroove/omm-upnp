/***************************************************************************|
|  OMM - Open Multimedia                                                    |
|                                                                           |
|  Copyright (C) 2011                                                       |
|  Jörg Bakker (jb'at'open-multimedia.org)                                  |
|                                                                           |
|  This file is part of OMM.                                                |
|                                                                           |
|  OMM is free software: you can redistribute it and/or modify              |
|  it under the terms of the GNU General Public License as published by     |
|  the Free Software Foundation version 3 of the License.                   |
|                                                                           |
|  OMM is distributed in the hope that it will be useful,                   |
|  but WITHOUT ANY WARRANTY; without even the implied warranty of           |
|  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            |
|  GNU General Public License for more details.                             |
|                                                                           |
|  You should have received a copy of the GNU General Public License        |
|  along with this program.  If not, see <http://www.gnu.org/licenses/>.    |
 ***************************************************************************/
#include <algorithm>

#include "QtDeviceGroupModel.h"

QtDeviceGroupModel::QtDeviceGroupModel(Omm::DeviceGroupInterface* pDeviceGroup, QObject *parent) :
QAbstractItemModel(parent),
_pDeviceGroup(pDeviceGroup),
_iconProvider(new QFileIconProvider())
{
    _charEncoding = QTextCodec::codecForName("UTF-8");
}


QtDeviceGroupModel::~QtDeviceGroupModel()
{
}


QVariant
QtDeviceGroupModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    
    if (index.internalPointer() == 0) {
        Omm::Log::instance()->upnp().warning("QtDeviceGroupModel::data() reference to device is 0:");
        return QVariant();
    }

    Omm::Device* pDevice = static_cast<Omm::Device*>(index.internalPointer());
    switch (role) {
        case Qt::DisplayRole:
            return QString(pDevice->getFriendlyName().c_str());
        case Qt::DecorationRole:
            return _iconProvider->icon(QFileIconProvider::Folder);
        default:
            return QVariant();
    }
}


Qt::ItemFlags
QtDeviceGroupModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;
    
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QVariant
QtDeviceGroupModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return "";
}


QModelIndex
QtDeviceGroupModel::index(int row, int column, const QModelIndex& parent) const
{
    return createIndex(row, column, _pDeviceGroup->getDevice(row));
}


QModelIndex
QtDeviceGroupModel::parent(const QModelIndex& index) const
{
    return QModelIndex();
}


int
QtDeviceGroupModel::rowCount(const QModelIndex& parent) const
{
    return _pDeviceGroup->getDeviceCount();
}


int
QtDeviceGroupModel::columnCount(const QModelIndex& parent) const
{
    return 1;
}


void
QtDeviceGroupModel::addDevice(int position, bool begin)
{
    Omm::Log::instance()->upnp().debug("Qt device group model adds device at position:" + Poco::NumberFormatter::format(position));

    if (begin) {
        beginInsertRows(QModelIndex(), position, position);
    }
    else {
        Omm::Log::instance()->upnp().debug("Qt device group model finished adding device.");
        endInsertRows();
        emit layoutChanged();
        if (rowCount() == 1) {
            emit setCurrentIndex(index(0, 0));
        }
    }
}


void
QtDeviceGroupModel::removeDevice(int position, bool begin)
{
    Omm::Log::instance()->upnp().debug("Qt device group model removes device at position:" + Poco::NumberFormatter::format(position));

    if (begin) {
        beginRemoveRows(QModelIndex(), position, position);
    }
    else {
        Omm::Log::instance()->upnp().debug("Qt device group model finished adding device.");
        endRemoveRows();
        emit layoutChanged();
    }
}
