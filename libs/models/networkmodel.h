/*
    Copyright 2013-2018 Jan Grulich <jgrulich@redhat.com>
    Copyright 2021 Wang Rui <wangrui@jingos.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PLASMA_NM_NETWORK_MODEL_H
#define PLASMA_NM_NETWORK_MODEL_H

#include <QAbstractListModel>

#include "networkitemslist.h"

#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/VpnConnection>
#include <NetworkManagerQt/WirelessDevice>
#include <NetworkManagerQt/Utils>

#if WITH_MODEMMANAGER_SUPPORT
#include <ModemManagerQt/modem.h>
#endif

class Q_DECL_EXPORT NetworkModel : public QAbstractListModel
{
Q_OBJECT

Q_PROPERTY(bool isAllowUpdate READ isAllowUpdate WRITE setAllowUpdate NOTIFY updateItemChanged);

public:
    bool isAllowUpdate() const { return m_isAllowUpdate; };
    void setAllowUpdate(const bool state);
    explicit NetworkModel(QObject *parent = nullptr);
    ~NetworkModel() override;

    enum ItemRole {
        ConnectionDetailsRole = Qt::UserRole + 1,
        ConnectionIconRole,
        ConnectionPathRole,
        ConnectionStateRole,
        DeviceName,
        DevicePathRole,
        DeviceStateRole,
        DuplicateRole,
        ItemUniqueNameRole,
        ItemTypeRole,
        LastUsedRole,
        LastUsedDateOnlyRole,
        NameRole,
        SecurityTypeRole,
        SecurityTypeStringRole,
        SectionRole,
        SignalRole,
        SlaveRole,
        SsidRole,
        SpecificPathRole,
        TimeStampRole,
        TypeRole,
        UniRole,
        UuidRole,
        VpnState,
        VpnType,
        RxBytesRole,
        TxBytesRole,
        
        IpAddressRole,
        SubnetMaskRole,
        RouterRole,
        NameServerRole,
        DNSSearchRole,
        AutoconnectRole,
        PasswordRole,
        GateWayRole,
        UpdateConnectRole,
        SaveAndActivedRole,
        KeyMgmtTypeRole,
        UpdateItemRole,
        SavedCountRole
    };
    Q_ENUMS(ItemRole)

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE int getSavedCount() const;

Q_SIGNALS:
    void updateItemChanged(bool state) const;
    void wirelessNetworkDisappearedChanged(const QString &ssid);

public Q_SLOTS:
    void onItemUpdated();
    void setDeviceStatisticsRefreshRateMs(const QString &devicePath, uint refreshRate);

private Q_SLOTS:
    void accessPointSignalStrengthChanged(int signal);
    void activeConnectionAdded(const QString &activeConnection);
    void activeConnectionRemoved(const QString &activeConnection);
    void activeConnectionStateChanged(NetworkManager::ActiveConnection::State state);
    void activeVpnConnectionStateChanged(NetworkManager::VpnConnection::State state,NetworkManager::VpnConnection::StateChangeReason reason);
    void availableConnectionAppeared(const QString &connection);
    void availableConnectionDisappeared(const QString &connection);
    void connectionAdded(const QString &connection);
    void connectionRemoved(const QString &connection);
    void connectionUpdated();
    void deviceAdded(const QString &device);
    void deviceRemoved(const QString &device);
    void deviceStateChanged(NetworkManager::Device::State state, NetworkManager::Device::State oldState, NetworkManager::Device::StateChangeReason reason);
#if WITH_MODEMMANAGER_SUPPORT
    void gsmNetworkAccessTechnologiesChanged(QFlags<MMModemAccessTechnology> accessTechnologies);
    void gsmNetworkCurrentModesChanged();
    void gsmNetworkSignalQualityChanged(const ModemManager::SignalQualityPair &signalQuality);
#endif
    void ipConfigChanged();
    void ipInterfaceChanged();
    void statusChanged(NetworkManager::Status status);
    void wirelessNetworkAppeared(const QString &ssid);
    void wirelessNetworkDisappeared(const QString &ssid);
    void wirelessNetworkSignalChanged(int signal);
    void wirelessNetworkReferenceApChanged(const QString &accessPoint);

    void initialize();
    
    
private:
    NetworkItemsList m_list;

    void addActiveConnection(const NetworkManager::ActiveConnection::Ptr &activeConnection);
    void addAvailableConnection(const QString &connection, const NetworkManager::Device::Ptr &device);
    void addConnection(const NetworkManager::Connection::Ptr &connection);
    void addDevice(const NetworkManager::Device::Ptr &device);
    void addWirelessNetwork(const NetworkManager::WirelessNetwork::Ptr &network, const NetworkManager::WirelessDevice::Ptr &device);
    void checkAndCreateDuplicate(const QString &connection, const QString &deviceUni);
    void initializeSignals();
    void initializeSignals(const NetworkManager::ActiveConnection::Ptr &activeConnection);
    void initializeSignals(const NetworkManager::Connection::Ptr &connection);
    void initializeSignals(const NetworkManager::Device::Ptr &device);
    void initializeSignals(const NetworkManager::WirelessNetwork::Ptr &network);
    void updateItem(NetworkModelItem *item);
    void updateFromWirelessNetwork(NetworkModelItem *item, const NetworkManager::WirelessNetwork::Ptr &network, const NetworkManager::WirelessDevice::Ptr &device);

    NetworkManager::WirelessSecurityType alternativeWirelessSecurity(const NetworkManager::WirelessSecurityType type);
    bool m_isAllowUpdate = true;
};

#endif // PLASMA_NM_NETWORK_MODEL_H
