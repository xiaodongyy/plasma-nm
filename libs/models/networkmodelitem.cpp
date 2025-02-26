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
#include "networkmodelitem.h"
#include "uiutils.h"

#include <NetworkManagerQt/AdslDevice>
#include <NetworkManagerQt/BluetoothDevice>
#include <NetworkManagerQt/BondDevice>
#include <NetworkManagerQt/BridgeDevice>
#include <NetworkManagerQt/InfinibandDevice>
#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/ModemDevice>
#include <NetworkManagerQt/Settings>
#include <NetworkManagerQt/TeamDevice>
#include <NetworkManagerQt/Utils>
#include <NetworkManagerQt/VlanDevice>
#include <NetworkManagerQt/VpnConnection>
#include <NetworkManagerQt/VpnSetting>
#include <NetworkManagerQt/WiredDevice>
#include <NetworkManagerQt/WirelessDevice>
#include <NetworkManagerQt/WirelessSetting>
#include <NetworkManagerQt/Ipv4Setting>
#include <NetworkManagerQt/ConnectionSettings>

#include <KLocalizedString>
#include <KUser>

#if WITH_MODEMMANAGER_SUPPORT
#include <ModemManagerQt/manager.h>
#include <ModemManagerQt/modem.h>
#include <ModemManagerQt/modemdevice.h>
#include <ModemManagerQt/modem3gpp.h>
#include <ModemManagerQt/modemcdma.h>
#endif
#include <KNotification>

NetworkModelItem::NetworkModelItem(QObject *parent)
    : QObject(parent)
    , m_connectionState(NetworkManager::ActiveConnection::Deactivated)
    , m_deviceState(NetworkManager::Device::UnknownState)
    , m_detailsValid(false)
    , m_duplicate(false)
    , m_mode(NetworkManager::WirelessSetting::Infrastructure)
    , m_securityType(NetworkManager::NoneSecurity)
    , m_signal(0)
    , m_slave(false)
    , m_type(NetworkManager::ConnectionSettings::Unknown)
    , m_vpnState(NetworkManager::VpnConnection::Unknown)
    , m_rxBytes(0)
    , m_txBytes(0)
    , m_handler(new Handler(this))
    , m_password("")
    , m_keyMgmtType(NetworkManager::WirelessSecuritySetting::WpaPsk)
{
}

NetworkModelItem::NetworkModelItem(const NetworkModelItem *item, QObject *parent)
    : QObject(parent)
    , m_connectionPath(item->connectionPath())
    , m_connectionState(NetworkManager::ActiveConnection::Deactivated)
    , m_detailsValid(false)
    , m_duplicate(true)
    , m_mode(item->mode())
    , m_name(item->name())
    , m_securityType(item->securityType())
    , m_slave(item->slave())
    , m_ssid(item->ssid())
    , m_timestamp(item->timestamp())
    , m_type(item->type())
    , m_uuid(item->uuid())
    , m_vpnState(NetworkManager::VpnConnection::Unknown)
    , m_rxBytes(0)
    , m_txBytes(0)
    , m_handler(new Handler(this))
    , m_router("Automatic")
    , m_autoConnect(true)
{
}

NetworkModelItem::~NetworkModelItem()

{
}

QString NetworkModelItem::activeConnectionPath() const
{
    return m_activeConnectionPath;
}

void NetworkModelItem::setActiveConnectionPath(const QString &path)
{
    m_activeConnectionPath = path;
}

QString NetworkModelItem::connectionPath() const
{
    return m_connectionPath;
}

void NetworkModelItem::setConnectionPath(const QString &path)
{
    if (m_connectionPath != path) {
        m_connectionPath = path;
        m_changedRoles << NetworkModel::ConnectionPathRole << NetworkModel::UniRole;
    }
}

NetworkManager::ActiveConnection::State NetworkModelItem::connectionState() const
{
    return m_connectionState;
}

void NetworkModelItem::setConnectionState(NetworkManager::ActiveConnection::State state)
{
    if (m_connectionState != state) {
        m_connectionState = state;
        m_changedRoles << NetworkModel::ConnectionStateRole << NetworkModel::SectionRole;
        refreshIcon();
    }
}

QStringList NetworkModelItem::details() const
{
    if (!m_detailsValid) {
        updateDetails();
    }
    return m_details;
}

QString NetworkModelItem::devicePath() const
{
    return m_devicePath;
}

QString NetworkModelItem::deviceName() const
{
    return m_deviceName;
}

void NetworkModelItem::setDeviceName(const QString &name)
{
    if (m_deviceName != name) {
        m_deviceName = name;
        m_changedRoles << NetworkModel::DeviceName;
    }
}

void NetworkModelItem::setDevicePath(const QString &path)
{
    if (m_devicePath != path) {
        m_devicePath = path;
        m_changedRoles << NetworkModel::DevicePathRole << NetworkModel::ItemTypeRole << NetworkModel::UniRole;
    }
}

QString NetworkModelItem::deviceState() const
{
    return UiUtils::connectionStateToString(m_deviceState);
}

void NetworkModelItem::setDeviceState(const NetworkManager::Device::State state)
{
    if (m_deviceState != state) {
        m_deviceState = state;
        m_changedRoles << NetworkModel::DeviceStateRole;
    }
}

bool NetworkModelItem::duplicate() const
{
    return m_duplicate;
}

void NetworkModelItem::setIcon(const QString& icon)
{
    if (icon != m_icon) {
        m_icon = icon;
        m_changedRoles << NetworkModel::ConnectionIconRole;
    }
}

void NetworkModelItem::refreshIcon()
{
    setIcon(computeIcon());
}

QString NetworkModelItem::computeIcon() const
{
    switch (m_type) {
        case NetworkManager::ConnectionSettings::Adsl:
            return QStringLiteral("network-mobile-100");
            break;
        case NetworkManager::ConnectionSettings::Bluetooth:
            if (m_connectionState == NetworkManager::ActiveConnection::Activated) {
                return QStringLiteral("network-bluetooth-activated");
            } else {
                return QStringLiteral("network-bluetooth");
            }
            break;
        case NetworkManager::ConnectionSettings::Bond:
            break;
        case NetworkManager::ConnectionSettings::Bridge:
            break;
        case NetworkManager::ConnectionSettings::Cdma:
        case NetworkManager::ConnectionSettings::Gsm:
            if (m_signal == 0 ) {
                return QStringLiteral("network-mobile-0");
            } else if (m_signal < 20) {
                return QStringLiteral("network-mobile-20");
            } else if (m_signal < 40) {
                return QStringLiteral("network-mobile-40");
            } else if (m_signal < 60) {
                return QStringLiteral("network-mobile-60");
            } else if (m_signal < 80) {
                return QStringLiteral("network-mobile-80");
            } else {
                return QStringLiteral("network-mobile-100");
            }
            break;
        case NetworkManager::ConnectionSettings::Infiniband:
            break;
        case NetworkManager::ConnectionSettings::OLPCMesh:
            break;
        case NetworkManager::ConnectionSettings::Pppoe:
            return QStringLiteral("network-mobile-100");
            break;
        case NetworkManager::ConnectionSettings::Vlan:
            break;
        case NetworkManager::ConnectionSettings::Vpn:
        case NetworkManager::ConnectionSettings::WireGuard:
            return QStringLiteral("network-vpn");
            break;
        case NetworkManager::ConnectionSettings::Wired:
            if (m_connectionState == NetworkManager::ActiveConnection::Activated) {
                return QStringLiteral("network-wired-activated");
            } else {
                return QStringLiteral("network-wired");
            }
            break;
        case NetworkManager::ConnectionSettings::Wireless:
            if (m_signal == 0 ) {
                if (m_mode == NetworkManager::WirelessSetting::Adhoc || m_mode == NetworkManager::WirelessSetting::Ap) {
                    return (m_securityType <= NetworkManager::NoneSecurity) ? QStringLiteral("network-wireless-100") : QStringLiteral("network-wireless-100-locked");
                }
                return (m_securityType <= NetworkManager::NoneSecurity) ? QStringLiteral("network-wireless-0") : QStringLiteral("network-wireless-0-locked");
            } else if (m_signal < 20) {
                return (m_securityType <= NetworkManager::NoneSecurity) ? QStringLiteral("network-wireless-20") : QStringLiteral("network-wireless-20-locked");
            } else if (m_signal < 40) {
                return (m_securityType <= NetworkManager::NoneSecurity) ? QStringLiteral("network-wireless-40") : QStringLiteral("network-wireless-40-locked");
            } else if (m_signal < 60) {
                return (m_securityType <= NetworkManager::NoneSecurity) ? QStringLiteral("network-wireless-60") : QStringLiteral("network-wireless-60-locked");
            } else if (m_signal < 80) {
                return (m_securityType <= NetworkManager::NoneSecurity) ? QStringLiteral("network-wireless-80") : QStringLiteral("network-wireless-80-locked");
            } else {
                return (m_securityType <= NetworkManager::NoneSecurity) ? QStringLiteral("network-wireless-100") : QStringLiteral("network-wireless-100-locked");
            }
            break;
        default:
            break;
    }

    if (m_connectionState == NetworkManager::ActiveConnection::Activated) {
        return QStringLiteral("network-wired-activated");
    } else {
        return QStringLiteral("network-wired");
    }
}

NetworkModelItem::ItemType NetworkModelItem::itemType() const
{
    if (!m_devicePath.isEmpty() ||
        m_type == NetworkManager::ConnectionSettings::Bond ||
        m_type == NetworkManager::ConnectionSettings::Bridge ||
        m_type == NetworkManager::ConnectionSettings::Vlan ||
        m_type == NetworkManager::ConnectionSettings::Team ||
        ((NetworkManager::status() == NetworkManager::Connected ||
          NetworkManager::status() == NetworkManager::ConnectedLinkLocal ||
          NetworkManager::status() == NetworkManager::ConnectedSiteOnly) && (m_type == NetworkManager::ConnectionSettings::Vpn || m_type == NetworkManager::ConnectionSettings::WireGuard))) {
        if (m_connectionPath.isEmpty() && m_type == NetworkManager::ConnectionSettings::Wireless) {
            return NetworkModelItem::AvailableAccessPoint;
        } else {
            return NetworkModelItem::AvailableConnection;
        }
    }
    return NetworkModelItem::UnavailableConnection;
}

NetworkManager::WirelessSetting::NetworkMode NetworkModelItem::mode() const
{
    return m_mode;
}

void NetworkModelItem::setMode(const NetworkManager::WirelessSetting::NetworkMode mode)
{
    if (m_mode != mode) {
        m_mode = mode;
        refreshIcon();
    }
}

QString NetworkModelItem::name() const
{
    return m_name;
}

void NetworkModelItem::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        m_changedRoles << NetworkModel::ItemUniqueNameRole << NetworkModel::NameRole;
    }
}

QString NetworkModelItem::originalName() const
{
    if (m_deviceName.isEmpty()) {
        return m_name;
    }
    return m_name % QLatin1String(" (") % m_deviceName % ')';
}

QString NetworkModelItem::sectionType() const
{
    if (m_connectionState == NetworkManager::ActiveConnection::Deactivated) {
        return "Available connections";
    }  else {
        return QString();
    }
}

NetworkManager::WirelessSecurityType NetworkModelItem::securityType() const
{
    return m_securityType;
}

void NetworkModelItem::setSecurityType(NetworkManager::WirelessSecurityType type)
{
    if (m_securityType != type) {
        m_securityType = type;
        m_changedRoles << NetworkModel::SecurityTypeStringRole << NetworkModel::SecurityTypeRole;
        refreshIcon();
    }
}

int NetworkModelItem::signal() const
{
    return m_signal;
}

void NetworkModelItem::setSignal(int signal)
{
    if (m_signal != signal) {
        m_signal = signal;
        m_changedRoles << NetworkModel::SignalRole;
        refreshIcon();
    }
}

bool NetworkModelItem::slave() const
{
    return m_slave;
}

void NetworkModelItem::setSlave(bool slave)
{
    if (m_slave != slave) {
        m_slave = slave;
        m_changedRoles << NetworkModel::SlaveRole;
    }
}

QString NetworkModelItem::specificPath() const
{
    return m_specificPath;
}

void NetworkModelItem::setSpecificPath(const QString &path)
{
    if (m_specificPath != path) {
        m_specificPath = path;
        m_changedRoles << NetworkModel::SpecificPathRole;
    }
}

QString NetworkModelItem::ssid() const
{
    return m_ssid;
}

void NetworkModelItem::setSsid(const QString &ssid)
{
    if (m_ssid != ssid) {
        m_ssid = ssid;
        m_changedRoles << NetworkModel::SsidRole << NetworkModel::UniRole;
    }
}

NetworkManager::ConnectionSettings::ConnectionType NetworkModelItem::type() const
{
    return m_type;
}

QDateTime NetworkModelItem::timestamp() const
{
    return m_timestamp;
}

void NetworkModelItem::setTimestamp(const QDateTime &date)
{
    if (m_timestamp != date) {
        m_timestamp = date;
        m_changedRoles << NetworkModel::TimeStampRole;
    }
}

void NetworkModelItem::setType(NetworkManager::ConnectionSettings::ConnectionType type)
{
    if (m_type != type) {
        m_type = type;
        m_changedRoles << NetworkModel::TypeRole << NetworkModel::ItemTypeRole << NetworkModel::UniRole;

        refreshIcon();
    }
}

QString NetworkModelItem::uni() const
{
    if (m_type == NetworkManager::ConnectionSettings::Wireless && m_uuid.isEmpty()) {
        return m_ssid + '%' + m_devicePath;
    } else {
        return m_connectionPath + '%' + m_devicePath;
    }
}

QString NetworkModelItem::uuid() const
{
    return m_uuid;
}

void NetworkModelItem::setUuid(const QString &uuid)
{
    if (m_uuid != uuid) {
        m_uuid = uuid;
        m_changedRoles << NetworkModel::UuidRole;
    }
}

QString NetworkModelItem::vpnState() const
{
    return UiUtils::vpnConnectionStateToString(m_vpnState);
}

void NetworkModelItem::setVpnState(NetworkManager::VpnConnection::State state)
{
    if (m_vpnState != state) {
        m_vpnState = state;
        m_changedRoles << NetworkModel::VpnState;
    }
}

QString NetworkModelItem::vpnType() const
{
    return m_vpnType;
}

void NetworkModelItem::setVpnType(const QString &type)
{
    if (m_vpnType != type) {
        m_vpnType = type;
        m_changedRoles << NetworkModel::VpnType;
    }
}

qulonglong NetworkModelItem::rxBytes() const
{
    return m_rxBytes;
}

void NetworkModelItem::setRxBytes(qulonglong bytes)
{
    if (m_rxBytes != bytes) {
        m_rxBytes = bytes;
        m_changedRoles << NetworkModel::RxBytesRole;
    }
}

qulonglong NetworkModelItem::txBytes() const
{
    return m_txBytes;
}

void NetworkModelItem::setTxBytes(qulonglong bytes)
{
    if (m_txBytes != bytes) {
        m_txBytes = bytes;
        m_changedRoles << NetworkModel::TxBytesRole;
    }
}

QString NetworkModelItem::ipAddress() const
{
    updateDetails();
    return m_ipAdress;
}

void NetworkModelItem::setIpAddress(const QString address)
{
    m_ipAdress = address;
}

QString NetworkModelItem::subnetMask() const
{
    updateDetails();
    return m_subnetMask;
}

void NetworkModelItem::setSubnetMask(const QString mask)
{
    m_subnetMask = mask;
}

QString NetworkModelItem::router() const
{
    return m_router;
}

void NetworkModelItem::setRouter(const QString router)
{
    m_router = router;
}

QString NetworkModelItem::dnsServer() const
{
    updateDetails();
    return m_dnsServer;
}

void NetworkModelItem::setDnsServer(const QString dnsServer)
{
    m_dnsServer = dnsServer;
}

QString NetworkModelItem::dnsSearch() const
{
    updateDetails();
    return m_dnsSearch;
}

void NetworkModelItem::setDnsSearch(const QString dnsSearch)
{
    m_dnsSearch = dnsSearch;
}

bool NetworkModelItem::autoConnect()
{
    updateDetails();
    return m_autoConnect;
}

void NetworkModelItem::setAutoConnect(const bool autoConnect)
{
    m_autoConnect = autoConnect;
    updateConnection();
}

QString NetworkModelItem::gateway()
{
    return m_gateway;
}

void NetworkModelItem::setGateway(const QString gateWay)
{
    m_gateway = gateWay;
}

QString NetworkModelItem::password()
{
    return m_password;
}

void NetworkModelItem::setPassword(const QString password)
{
    m_password = password;
}

NetworkManager::WirelessSecuritySetting::KeyMgmt NetworkModelItem::keyMgmtType()
{
    return m_keyMgmtType;
}

void NetworkModelItem::setKeyMgmtType(const NetworkManager::WirelessSecuritySetting::KeyMgmt type)
{
    m_keyMgmtType = type;
}

void NetworkModelItem::initPassword()
{
    NetworkManager::Connection::Ptr connection = NetworkManager::findConnection(m_connectionPath);
    if(!connection) {
        return;
    }

    NetworkManager::WirelessSecuritySetting::Ptr wifiSecurity = connection->settings()->setting(NetworkManager::Setting::WirelessSecurity).staticCast<NetworkManager::WirelessSecuritySetting>();
    QStringList requiredSecrets = wifiSecurity->needSecrets();
    QVariantMap setting = wifiSecurity->toMap();
    QString settingName = QLatin1String("802-11-wireless-security");
    bool requestSecrets = false;

    for (const QString &secret : requiredSecrets) {
            if (setting.contains(secret + QLatin1String("-flags"))) {
                NetworkManager::Setting::SecretFlagType secretFlag = (NetworkManager::Setting::SecretFlagType)setting.value(secret + QLatin1String("-flags")).toInt();
                if (secretFlag == NetworkManager::Setting::None || secretFlag == NetworkManager::Setting::AgentOwned) {
                    requestSecrets = true;
                }
            } else {
                requestSecrets = true;
            }
    }

    QDBusPendingReply<NMVariantMapMap> reply;
    if (requestSecrets) {
        reply = connection->secrets(settingName);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
        watcher->setProperty("connection", connection->name());
        watcher->setProperty("settingName", settingName);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, &NetworkModelItem::replyFinishedPassword);
    }
}

void NetworkModelItem::updateConnection()
{
    NetworkManager::Connection::Ptr connection = NetworkManager::findConnection(m_connectionPath);
    if(!connection){
        return;
    }

    NetworkManager::WirelessSecuritySetting::Ptr wifiSecurity = connection->settings()->setting(NetworkManager::Setting::WirelessSecurity).staticCast<NetworkManager::WirelessSecuritySetting>();
    QStringList requiredSecrets = wifiSecurity->needSecrets();
    QVariantMap setting = wifiSecurity->toMap();
    QString settingName = QLatin1String("802-11-wireless-security");
    bool requestSecrets = false;

    for (const QString &secret : requiredSecrets) {
        if (setting.contains(secret + QLatin1String("-flags"))) {
            NetworkManager::Setting::SecretFlagType secretFlag = (NetworkManager::Setting::SecretFlagType)setting.value(secret + QLatin1String("-flags")).toInt();
            if (secretFlag == NetworkManager::Setting::None || secretFlag == NetworkManager::Setting::AgentOwned) {
                requestSecrets = true;
            }
        } else {
            requestSecrets = true;
        }
    }

    QDBusPendingReply<NMVariantMapMap> reply;
    if (requestSecrets) {
        reply = connection->secrets(settingName);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
        watcher->setProperty("connection", connection->name());
        watcher->setProperty("settingName", settingName);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, &NetworkModelItem::replyFinished);
    }
}

bool NetworkModelItem::saveAndActived(const QString pwd)
{
    m_handler->addAndActivateConnection(m_devicePath,m_specificPath,pwd);
    return true;
}

bool NetworkModelItem::operator==(const NetworkModelItem *item) const
{
    if (!item->uuid().isEmpty() && !uuid().isEmpty()) {
        if (item->devicePath() == devicePath() && item->uuid() == uuid()) {
            return true;
        }
    } else if (item->type() == NetworkManager::ConnectionSettings::Wireless && type() == NetworkManager::ConnectionSettings::Wireless) {
        if (item->ssid() == ssid() && item->devicePath() == devicePath()) {
            return true;
        }
    }

    return false;
}

void NetworkModelItem::invalidateDetails()
{
    m_detailsValid = false;
    m_changedRoles << NetworkModel::ConnectionDetailsRole;
}

void NetworkModelItem::updateDetails() const
{
    m_detailsValid = true;
    m_details.clear();

    if (itemType() == NetworkModelItem::UnavailableConnection) {
        return;
    }

    NetworkManager::Device::Ptr device = NetworkManager::findNetworkInterface(m_devicePath);

    // Get IPv[46]Address and related nameservers + IPv4 default gateway
    if (device && device->ipV4Config().isValid() && m_connectionState == NetworkManager::ActiveConnection::Activated) {
        if (!device->ipV4Config().addresses().isEmpty()) {
            QHostAddress addr = device->ipV4Config().addresses().first().ip();
            if (!addr.isNull()) {
                m_details << i18n("IPv4 Address") << addr.toString();
                
                m_ipAdress = addr.toString();
            }
        }
        if (!device->ipV4Config().gateway().isEmpty()) {
            QString addr = device->ipV4Config().gateway();

            if (!addr.isNull()) {
                m_gateway = device->ipV4Config().gateway();
                m_details << i18n("IPv4 Default Gateway") << addr;
            }
        }
        if (!device->ipV4Config().nameservers().isEmpty()) {
            QHostAddress addr = device->ipV4Config().nameservers().first();
            if (!addr.isNull()) {
                m_details << i18n("IPv4 Nameserver") << addr.toString();
                m_dnsServer = addr.toString();
            }
        }
    }

    if (device && device->ipV6Config().isValid() && m_connectionState == NetworkManager::ActiveConnection::Activated) {
        if (!device->ipV6Config().addresses().isEmpty()) {
            QHostAddress addr = device->ipV6Config().addresses().first().ip();
            if (!addr.isNull()) {
                m_details << i18n("IPv6 Address") << addr.toString();
            }
        }
        if (!device->ipV6Config().nameservers().isEmpty()) {
            QHostAddress addr = device->ipV6Config().nameservers().first();
            if (!addr.isNull()) {
                m_details << i18n("IPv6 Nameserver") << addr.toString();
            }
        }
    }
    if (m_type == NetworkManager::ConnectionSettings::Wired) {
        NetworkManager::WiredDevice::Ptr wiredDevice = device.objectCast<NetworkManager::WiredDevice>();
        if (wiredDevice) {
            if (m_connectionState == NetworkManager::ActiveConnection::Activated) {
                m_details << i18n("Connection speed") << UiUtils::connectionSpeed(wiredDevice->bitRate());
            }
            m_details << i18n("MAC Address") << wiredDevice->permanentHardwareAddress();
        }
    } else if (m_type == NetworkManager::ConnectionSettings::Wireless) {
        NetworkManager::WirelessDevice::Ptr wirelessDevice = device.objectCast<NetworkManager::WirelessDevice>();
        m_details << i18n("Access point (SSID)") << m_ssid;
        if (m_mode == NetworkManager::WirelessSetting::Infrastructure) {
            m_details << i18n("Signal strength") << QStringLiteral("%1%").arg(m_signal);
        }
        m_details << i18n("Security type") << UiUtils::labelFromWirelessSecurity(m_securityType);
        if (wirelessDevice) {
            if (m_connectionState == NetworkManager::ActiveConnection::Activated) {
                m_details << i18n("Connection speed") << UiUtils::connectionSpeed(wirelessDevice->bitRate());
            }
            m_details << i18n("MAC Address") << wirelessDevice->permanentHardwareAddress();
        }
    } else if (m_type == NetworkManager::ConnectionSettings::Gsm || m_type == NetworkManager::ConnectionSettings::Cdma) {
#if WITH_MODEMMANAGER_SUPPORT
        NetworkManager::ModemDevice::Ptr modemDevice = device.objectCast<NetworkManager::ModemDevice>();
        if (modemDevice) {
            ModemManager::ModemDevice::Ptr modem = ModemManager::findModemDevice(modemDevice->udi());
            if (modem) {
                ModemManager::Modem::Ptr modemNetwork = modem->interface(ModemManager::ModemDevice::ModemInterface).objectCast<ModemManager::Modem>();

                if (m_type == NetworkManager::ConnectionSettings::Gsm) {
                    ModemManager::Modem3gpp::Ptr gsmNet = modem->interface(ModemManager::ModemDevice::GsmInterface).objectCast<ModemManager::Modem3gpp>();
                    if (gsmNet) {
                        m_details << i18n("Operator") << gsmNet->operatorName();
                    }
                } else {
                    ModemManager::ModemCdma::Ptr cdmaNet = modem->interface(ModemManager::ModemDevice::CdmaInterface).objectCast<ModemManager::ModemCdma>();
                    m_details << i18n("Network ID") << QString("%1").arg(cdmaNet->nid());
                }

                if (modemNetwork) {
                    m_details << i18n("Signal Quality") << QString("%1%").arg(modemNetwork->signalQuality().signal);
                    m_details << i18n("Access Technology") << UiUtils::convertAccessTechnologyToString(modemNetwork->accessTechnologies());
                }
            }
        }
#endif
    } else if (m_type == NetworkManager::ConnectionSettings::Vpn) {
        m_details << i18n("VPN plugin") << m_vpnType;

        if (m_connectionState == NetworkManager::ActiveConnection::Activated) {
            NetworkManager::ActiveConnection::Ptr active = NetworkManager::findActiveConnection(m_activeConnectionPath);
            NetworkManager::VpnConnection::Ptr vpnConnection;

            if (active) {
                vpnConnection = NetworkManager::VpnConnection::Ptr(new NetworkManager::VpnConnection(active->path()), &QObject::deleteLater);
            }

            if (vpnConnection && !vpnConnection->banner().isEmpty()) {
                m_details << i18n("Banner") << vpnConnection->banner().simplified();
            }
        }
    } else if (m_type == NetworkManager::ConnectionSettings::Bluetooth) {
        NetworkManager::BluetoothDevice::Ptr bluetoothDevice = device.objectCast<NetworkManager::BluetoothDevice>();
        if (bluetoothDevice) {
            m_details << i18n("Name") << bluetoothDevice->name();
            if (bluetoothDevice->bluetoothCapabilities() == NetworkManager::BluetoothDevice::Pan) {
                m_details << i18n("Capabilities") << QStringLiteral("PAN");
            } else if (bluetoothDevice->bluetoothCapabilities() == NetworkManager::BluetoothDevice::Dun) {
                m_details << i18n("Capabilities") << QStringLiteral("DUN");
            }
            m_details << i18n("MAC Address") << bluetoothDevice->hardwareAddress();

        }
    } else if (m_type == NetworkManager::ConnectionSettings::Infiniband) {
        NetworkManager::InfinibandDevice::Ptr infinibandDevice = device.objectCast<NetworkManager::InfinibandDevice>();
        m_details << i18n("Type") << i18n("Infiniband");
        if (infinibandDevice) {
            m_details << i18n("MAC Address") << infinibandDevice->hwAddress();
        }
    } else if (m_type == NetworkManager::ConnectionSettings::Bond) {
        NetworkManager::BondDevice::Ptr bondDevice = device.objectCast<NetworkManager::BondDevice>();
        m_details << i18n("Type") << i18n("Bond");
        if (bondDevice) {
            m_details << i18n("MAC Address") << bondDevice->hwAddress();
        }
    } else if (m_type == NetworkManager::ConnectionSettings::Bridge) {
        NetworkManager::BridgeDevice::Ptr bridgeDevice = device.objectCast<NetworkManager::BridgeDevice>();
        m_details << i18n("Type") << i18n("Bridge");
        if (bridgeDevice) {
            m_details << i18n("MAC Address") << bridgeDevice->hwAddress();
        }
    } else if (m_type == NetworkManager::ConnectionSettings::Vlan) {
        NetworkManager::VlanDevice::Ptr vlanDevice = device.objectCast<NetworkManager::VlanDevice>();
        m_details << i18n("Type") << i18n("Vlan");
        if (vlanDevice) {
            m_details << i18n("Vlan ID") << QString("%1").arg(vlanDevice->vlanId());
            m_details << i18n("MAC Address") << vlanDevice->hwAddress();
        }
    } else if (m_type == NetworkManager::ConnectionSettings::Adsl) {
        m_details << i18n("Type") << i18n("Adsl");
    } else if (m_type == NetworkManager::ConnectionSettings::Team) {
        NetworkManager::TeamDevice::Ptr teamDevice = device.objectCast<NetworkManager::TeamDevice>();
        m_details << i18n("Type") << i18n("Team");
        if (teamDevice) {
            m_details << i18n("MAC Address") << teamDevice->hwAddress();
        }
    }

    if (device && m_connectionState == NetworkManager::ActiveConnection::Activated) {
        m_details << i18n("Device") << device->interfaceName();
    }

    NetworkManager::Connection::Ptr connection = NetworkManager::findConnection(m_connectionPath);
    if (connection) {
        NetworkManager::ConnectionSettings::Ptr connectionSettings = connection->settings();
        m_autoConnect = connectionSettings->autoconnect();
        NetworkManager::Setting::Ptr settings = connectionSettings->setting(NetworkManager::Setting::Ipv4);
        NetworkManager::Ipv4Setting::Ptr ipv4Setting = settings.staticCast<NetworkManager::Ipv4Setting>();
        
        switch (ipv4Setting->method()) {
            case NetworkManager::Ipv4Setting::Automatic:
                m_router = "Automatic";
                break;
            case NetworkManager::Ipv4Setting::Manual:
                m_router = "Manual";
                break;
        }

        m_dnsSearch = ipv4Setting->dnsSearch().join(",");
        for (const NetworkManager::IpAddress &addr : ipv4Setting->addresses()) {
            m_subnetMask = addr.netmask().toString();
            break;
        }
    }
}

void NetworkModelItem::replyFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<NMVariantMapMap> reply = *watcher;
    const QString settingName = watcher->property("settingName").toString();

    if (reply.isValid()) {
        NMVariantMapMap secrets = reply.argumentAt<0>();

        for (const QString &key : secrets.keys()) {
            if (key == settingName) {
                NetworkManager::Connection::Ptr connection = NetworkManager::findConnection(m_connectionPath);
                NetworkManager::Setting::Ptr setting = connection->settings()->setting(NetworkManager::Setting::typeFromString(key));
                if (setting) {
                    setting->secretsFromMap(secrets.value(key));

                    NetworkManager::WirelessSecuritySetting::Ptr wifiSecurity = setting.staticCast<NetworkManager::WirelessSecuritySetting>();
                    QVariantMap wssMap = wifiSecurity->toMap();
                    NetworkManager::ConnectionSettings::Ptr connectionSettings = connection->settings();
                    connectionSettings->setAutoconnect(m_autoConnect);
                    NetworkManager::Ipv4Setting::Ptr m_tmpIpv4Setting = NetworkManager::Ipv4Setting::Ptr(new NetworkManager::Ipv4Setting());
                    NetworkManager::Ipv4Setting::Ptr ipv4Setting = connectionSettings->setting(NetworkManager::Setting::Ipv4).staticCast<NetworkManager::Ipv4Setting>();
                    
                    m_tmpIpv4Setting->setRouteMetric(ipv4Setting->routeMetric());
                    m_tmpIpv4Setting->setRoutes(ipv4Setting->routes());
                    m_tmpIpv4Setting->setNeverDefault(ipv4Setting->neverDefault());
                    m_tmpIpv4Setting->setIgnoreAutoRoutes(ipv4Setting->ignoreAutoRoutes());
                    m_tmpIpv4Setting->setDhcpHostname(ipv4Setting->dhcpHostname());
                    m_tmpIpv4Setting->setDhcpSendHostname(ipv4Setting->dhcpSendHostname());
                    m_tmpIpv4Setting->setDadTimeout(ipv4Setting->dadTimeout());

                    if(m_router == "Automatic") {
                        m_tmpIpv4Setting->setMethod(NetworkManager::Ipv4Setting::Automatic);
                    } else {
                        QList<NetworkManager::IpAddress> list;
                        m_tmpIpv4Setting->setMethod(NetworkManager::Ipv4Setting::Manual);
                        NetworkManager::IpAddress address;

                        if(!m_ipAdress.isEmpty()) {
                            address.setIp(QHostAddress(m_ipAdress));
                        }
                        if(!m_subnetMask.isEmpty()) {
                            address.setNetmask(QHostAddress(m_subnetMask));
                        }else{
                            address.setNetmask(QHostAddress("255.255.0.0"));
                        }
                        if(!m_ipAdress.isEmpty()) {
                            address.setGateway(QHostAddress(m_gateway));
                        }
                         
                        list << address;
                        m_tmpIpv4Setting->setAddresses(list);
                        QList<QHostAddress> tmpAddrList;
                        QHostAddress addr(m_dnsServer);

                        if (!addr.isNull()) {
                            tmpAddrList.append(addr);
                            m_tmpIpv4Setting->setDns(tmpAddrList);
                        }
                        if(!m_dnsSearch.isEmpty()) {
                            m_tmpIpv4Setting->setDnsSearch(m_dnsSearch.split(','));
                        }    
                    }

                    NMVariantMapMap csMapMap = connectionSettings->toMap();
                    QVariantMap ipv4Map = m_tmpIpv4Setting->toMap();

                    csMapMap.insert(NetworkManager::Setting::typeAsString(NetworkManager::Setting::Ipv4), ipv4Map);
                    csMapMap.insert(NetworkManager::Setting::typeAsString(NetworkManager::Setting::WirelessSecurity), wssMap);
                    m_handler->updateConnection(connection,csMapMap);
                }
            }
        }
    } else {
        KNotification *notification = new KNotification("FailedToGetSecrets", KNotification::CloseOnTimeout);
        notification->setComponentName("networkmanagement");
        notification->setTitle(i18n("Failed to get secrets for %1", watcher->property("connection").toString()));
        notification->setText(reply.error().message());
        notification->setIconName(QStringLiteral("dialog-warning"));
        notification->sendEvent();
    }
    watcher->deleteLater();
    // We should be now fully with secrets
}

void NetworkModelItem::replyFinishedPassword(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<NMVariantMapMap> reply = *watcher;
    const QString settingName = watcher->property("settingName").toString();
    if (reply.isValid()) {
        NMVariantMapMap secrets = reply.argumentAt<0>();
        for (const QString &key : secrets.keys()) {
            if (key == settingName) {
                NetworkManager::Connection::Ptr connection = NetworkManager::findConnection(m_connectionPath);
                NetworkManager::Setting::Ptr setting = connection->settings()->setting(NetworkManager::Setting::typeFromString(key));
                if (setting) {
                    setting->secretsFromMap(secrets.value(key));
                    NetworkManager::WirelessSecuritySetting::Ptr wifiSecurity = setting.staticCast<NetworkManager::WirelessSecuritySetting>();
                    m_password = wifiSecurity->psk();
                }
            }
        }
    } else {
        KNotification *notification = new KNotification("FailedToGetSecrets", KNotification::CloseOnTimeout);
        notification->setComponentName("networkmanagement");
        notification->setTitle(i18n("Failed to get secrets for %1", watcher->property("connection").toString()));
        notification->setText(reply.error().message());
        notification->setIconName(QStringLiteral("dialog-warning"));
        notification->sendEvent();
    }
    watcher->deleteLater();
    // We should be now fully with secrets
}

