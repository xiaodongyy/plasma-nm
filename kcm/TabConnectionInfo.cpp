#include "TabConnectionInfo.h"
#include "ui_TabConnectionInfo.h"

TabConnectionInfo::TabConnectionInfo(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TabConnectionInfo)
{
    ui->setupUi(this);
}

TabConnectionInfo::~TabConnectionInfo()
{
    delete ui;
}

void TabConnectionInfo::setConnection(const NetworkManager::Connection::Ptr &connection)
{
    if (connection) {
        QDateTime dateTime = connection->settings()->timestamp();
        ui->lastUsedL->setText(KGlobal::locale()->formatDateTime(dateTime));
    }
}
