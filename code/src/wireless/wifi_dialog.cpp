
#include "onyx/wireless/wifi_dialog.h"
#include "onyx/screen/screen_update_watcher.h"
#include "onyx/data/data.h"
#include "onyx/wireless/ap_conf_dialog.h"

namespace ui
{
const int SPACING = 2;
const int WIDGET_HEIGHT = 36;
static const int AP_ITEM_HEIGHT = 80;
static const int MARGINS = 10;

static const QString BUTTON_STYLE =    "\
QPushButton                             \
{                                       \
    background: transparent;            \
    font-size: 14px;                    \
    border-width: 1px;                  \
    border-color: transparent;          \
    border-style: solid;                \
    color: black;                       \
    padding: 0px;                       \
}                                       \
QPushButton:pressed                     \
{                                       \
    padding-left: 0px;                  \
    padding-top: 0px;                   \
    border-color: black;                \
    background-color: black;            \
}                                       \
QPushButton:checked                     \
{                                       \
    padding-left: 0px;                  \
    padding-top: 0px;                   \
    color: white;                       \
    border-color: black;                \
    background-color: black;            \
}                                       \
QPushButton:disabled                    \
{                                       \
    padding-left: 0px;                  \
    padding-top: 0px;                   \
    border-color: dark;                 \
    color: dark;                        \
    background-color: white;            \
}";

class WifiViewFactory : public ui::Factory
{
public:
    WifiViewFactory(){}
    ~WifiViewFactory(){}

public:
    virtual ContentView * createView(QWidget *parent, const QString &type = QString())
    {
        return new WifiAPItem(parent);
    }
};

static WifiViewFactory my_factory;

WifiDialog::WifiDialog(QWidget *parent,
                       SysStatus & sys)
#ifndef Q_WS_QWS
    : QDialog(parent, 0)
#else
    : QDialog(parent, Qt::FramelessWindowHint)
#endif
    , big_box_(this)
    , title_hbox_(0)
    , content_layout_(0)
    , ap_layout_(0)
    , buttons_layout_(0)
    , title_icon_label_(0)
    , title_text_label_(tr("Wireless Connections"), 0)
    , close_button_("", 0)
    , state_widget_(0)
    , prev_button_(QPixmap(":/images/prev_page.png"), "", 0)
    , next_button_(QPixmap(":/images/next_page.png"), "", 0)
    , ap_view_(&my_factory)
    , sys_(sys)
    , proxy_(sys.connectionManager())
    , ap_dialog_visible_(false)
{
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Base);

    createLayout();
    setupConnections();
}

WifiDialog::~WifiDialog()
{
}

int  WifiDialog::popup(bool scan)
{
    showMaximized();
    scanResults(scan_results_);
    arrangeAPItems(scan_results_);
    onyx::screen::instance().flush(this, onyx::screen::ScreenProxy::GC);
    if (scan)
    {
        triggerScan();
    }
    return exec();
}

void WifiDialog::keyPressEvent(QKeyEvent *)
{
}

void WifiDialog::keyReleaseEvent(QKeyEvent *ke)
{
    if (ke->key() == Qt::Key_Escape)
    {
        ke->accept();
        reject();
    }
}

bool WifiDialog::event(QEvent * e)
{
    bool ret = QDialog::event(e);
    if (e->type() == QEvent::UpdateRequest)
    {
        onyx::screen::watcher().updateScreen();
        e->accept();
        return true;
    }
    return ret;
}

void WifiDialog::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.fillRect(title_hbox_.contentsRect(), QBrush(Qt::black));

    QPainterPath path;
    path.addRoundedRect(content_layout_.contentsRect().adjusted(2, 2, -2, -2), 8, 8, Qt::AbsoluteSize);
    painter.fillPath(path, QBrush(QColor(230, 230, 230)));
    painter.setPen(QColor(Qt::black));
    painter.drawPath(path);
}

void WifiDialog::resizeEvent(QResizeEvent *re)
{
    QDialog::resizeEvent(re);
}

void WifiDialog::mousePressEvent(QMouseEvent *)
{
}

void WifiDialog::mouseReleaseEvent(QMouseEvent *)
{
}

void WifiDialog::createLayout()
{
    // big_box_.setSizeConstraint(QLayout::SetMinimumSize);
    big_box_.setSpacing(SPACING);
    big_box_.setContentsMargins(SPACING, SPACING, SPACING, SPACING);

    // title hbox.
    big_box_.addLayout(&title_hbox_);
    title_hbox_.setContentsMargins(0, 0, 0, 0);
    title_hbox_.addWidget(&title_icon_label_, 0, Qt::AlignVCenter);
    title_hbox_.addSpacing(SPACING * 2);
    title_hbox_.addWidget(&title_text_label_, 0, Qt::AlignVCenter);
    title_hbox_.addStretch(0);
    title_hbox_.addWidget(&close_button_);
    title_icon_label_.setPixmap(QPixmap(":/images/network_connection.png"));
    title_text_label_.setAlignment(Qt::AlignVCenter);
    title_icon_label_.setFixedHeight(WIDGET_HEIGHT);
    title_text_label_.useTitleBarStyle();
    title_text_label_.setFixedHeight(WIDGET_HEIGHT);

    close_button_.setStyleSheet(BUTTON_STYLE);
    QPixmap close_pixmap(":/images/close.png");
    close_button_.setIconSize(close_pixmap.size());
    close_button_.setIcon(QIcon(close_pixmap));
    close_button_.setFocusPolicy(Qt::NoFocus);
    QObject::connect(&close_button_, SIGNAL(clicked()), this, SLOT(onCloseClicked()));

    // content layout.
    big_box_.addLayout(&content_layout_);
    content_layout_.setContentsMargins(MARGINS, MARGINS, MARGINS, MARGINS);

    // Status.
    content_layout_.addWidget(&state_widget_);
    QObject::connect(&state_widget_, SIGNAL(refreshClicked()), this, SLOT(onRefreshClicked()));
    QObject::connect(&state_widget_, SIGNAL(customizedClicked()), this, SLOT(onCustomizedClicked()));
    QObject::connect(&prev_button_, SIGNAL(clicked()), &ap_view_, SLOT(goPrev()), Qt::QueuedConnection);
    QObject::connect(&next_button_, SIGNAL(clicked()), &ap_view_, SLOT(goNext()), Qt::QueuedConnection);

    // ap layout.
    ap_layout_.setContentsMargins(MARGINS, MARGINS, MARGINS, MARGINS);
    ap_layout_.setSpacing(5);
    content_layout_.addLayout(&ap_layout_);
    ap_layout_.addWidget(&ap_view_);

    QObject::connect(&ap_view_, SIGNAL(positionChanged(const int, const int)), this, SLOT(onPositionChanged(const int, const int)));
    QObject::connect(&ap_view_, SIGNAL(itemActivated(CatalogView*, ContentView*, int)), this, SLOT(onItemActivated(CatalogView*, ContentView*, int)));

    ap_view_.setPreferItemSize(QSize(-1, AP_ITEM_HEIGHT));
    ap_view_.setNeighbor(&state_widget_.dashBoard(), CatalogView::UP);
    content_layout_.addSpacing(50);

    // Buttons.
    content_layout_.addLayout(&buttons_layout_);
    buttons_layout_.setContentsMargins(MARGINS, 0, MARGINS, 0);
    prev_button_.setFocusPolicy(Qt::NoFocus);
    next_button_.setFocusPolicy(Qt::NoFocus);
    buttons_layout_.addWidget(&prev_button_);
    buttons_layout_.addStretch(0);
    buttons_layout_.addWidget(&next_button_);
    showPaginationButtons(true, true);

    // Hardware address.
    hardware_address_.setFixedHeight(WIDGET_HEIGHT);
    hardware_address_.setContentsMargins(MARGINS, 0, MARGINS, 0);
    content_layout_.addWidget(&hardware_address_);
}

void WifiDialog::scanResults(WifiProfiles &aps)
{
    proxy_.scanResults(aps);

#ifdef _WINDOWS
    aps.clear();
    for(int i = 0; i < 10; ++i)
    {
        WifiProfile a;
        a.setSsid(QString::number(i));
        QByteArray d("aa:bb:cc:dd:ee:ff");
        a.setBssid(d);
        aps.push_back(a);
    }
#endif

}

void WifiDialog::arrangeAPItems(WifiProfiles & profiles)
{
    clearDatas(datas_);
    foreach(WifiProfile profile, profiles)
    {
        ODataPtr d (new OData(profile));
        datas_.push_back(d);
    }
    ap_view_.setData(datas_, true);
    showPaginationButtons(ap_view_.hasPrev(), ap_view_.hasNext());
}

void WifiDialog::setupConnections()
{
    QObject::connect(&proxy_, SIGNAL(connectionChanged(WifiProfile,WpaConnection::ConnectionState)),
        this, SLOT(onConnectionChanged(WifiProfile, WpaConnection::ConnectionState)));
    QObject::connect(&proxy_, SIGNAL(passwordRequired(WifiProfile )),
            this, SLOT(onNeedPassword(WifiProfile )));
    QObject::connect(&proxy_, SIGNAL(noMatchedAP()),
            this, SLOT(onNoMatchedAP()));

    QObject::connect(&sys_, SIGNAL(sdioChangedSignal(bool)), this, SLOT(onSdioChanged(bool)));
}

void WifiDialog::clear()
{
}

void WifiDialog::scan()
{
    proxy_.start();
}

void WifiDialog::triggerScan()
{
    proxy_.start();
}

void WifiDialog::connect(const QString & ssid, const QString &password)
{
    proxy_.scanResults(scan_results_);
    foreach(WifiProfile profile, scan_results_)
    {
        if (profile.ssid() == ssid)
        {
            setPassword(profile, password);
            onAPItemClicked(profile);
            return;
        }
    }
}

// So far, disable the auto connecting to best ap.
bool WifiDialog::connectToBestAP()
{
    if (!auto_connect_to_best_ap_)
    {
        return false;
    }
    auto_connect_to_best_ap_ = false;

    sys::SystemConfig conf;
    WifiProfiles all = records(conf);
    if (all.size() <= 0)
    {
        return false;
    }
    sortByCount(all);
    if (all.front().count() <= 0)
    {
        return false;
    }
    onAPItemClicked(all.front());
    return true;
}

bool WifiDialog::connectToDefaultAP()
{
    if (!auto_connect_to_default_ap_)
    {
        return false;
    }
    auto_connect_to_default_ap_ = false;

    QString ap = sys::SystemConfig::defaultAccessPoint();
    if (!ap.isEmpty())
    {
        for(int i = 0; i < scan_results_.size(); ++i)
        {
            if (scan_results_[i].ssid().contains(ap, Qt::CaseInsensitive))
            {
                onAPItemClicked(scan_results_[i]);
                return true;
            }
        }
    }
    return false;
}

void WifiDialog::onScanTimeout()
{
    // If we can not connect to wpa supplicant before, we need to
    // scan now.
    scan();
}

void WifiDialog::onConnectionTimeout()
{
    // Need to clean up password.
    // Timeout in wifi dialog.
    updateStateLable(WpaConnection::STATE_TIMEOUT);
}

void WifiDialog::onAPItemClicked(WifiProfile & profile)
{
    proxy_.connectTo(profile);
    update();
}

/// Refresh handler. We try to scan if wpa_supplicant has been launched.
/// Otherwise, we need to make sure wpa_supplicant is launched and it
/// acquired the system bus.
void WifiDialog::onRefreshClicked()
{
    proxy_.start();
}

void WifiDialog::onCustomizedClicked()
{
    sys::WifiProfile profile;
    if (showConfigurationDialog(profile))
    {
        onAPItemClicked(profile);
    }
}

void WifiDialog::onCloseClicked()
{
    reject();
}

void WifiDialog::onSdioChanged(bool on)
{
    if (on)
    {
        onRefreshClicked();
        enableChildren(on);
        updateHardwareAddress();
    }
    else
    {
        updateStateLable(WpaConnection::STATE_DISABLED);
        enableChildren(on);
    }
}

void WifiDialog::enableChildren(bool enable)
{
    state_widget_.setEnabled(enable);
    ap_view_.setEnabled(enable);
}

void WifiDialog::onScanReturned()
{
    proxy_.scanResults(scan_results_);
    arrangeAPItems(scan_results_);
}

void WifiDialog::onConnectionChanged(WifiProfile profile, WpaConnection::ConnectionState state)
{
    updateStateLable(state);
    if (state == WpaConnection::STATE_CONNECTED)
    {
    }
    if (state == WpaConnection::STATE_COMPLETE)
    {
    }
    else if (state == WpaConnection::STATE_ACQUIRING_ADDRESS_ERROR)
    {
    }
    else if (state == WpaConnection::STATE_AUTHENTICATION_FAILED)
    {
    }
    else if (state == WpaConnection::STATE_SCANNED)
    {
        onScanReturned();
    }
    else if (state == WpaConnection::STATE_CONNECTING)
    {
        // Mark the item selected.
        qDebug("connecting to %s", qPrintable(profile.ssid()));
    }
    onyx::screen::watcher().enqueue(this, onyx::screen::ScreenProxy::GU);
}

void WifiDialog::updateStateLable(WpaConnection::ConnectionState state)
{
    // If wifi is disabled.
    //if (!wifi_enabled_ && state != WpaConnection::STATE_DISABLED)
    //{
    //    return;
    //}

    // qDebug("WifiDialog::updateStateLable %d", state);
    switch (state)
    {
    case WpaConnection::STATE_DISABLED:
        state_widget_.setState(tr("Wifi is disabled."));
        break;
    case WpaConnection::STATE_HARDWARE_ERROR:
        state_widget_.setState(tr("Can not start wifi device."));
        break;
    case WpaConnection::STATE_SCANNING:
        state_widget_.setState(tr("Scanning..."));
        break;
    case WpaConnection::STATE_SCANNED:
        //if (!isConnecting())
        {
            state_widget_.setState(tr("Ready"));
        }
        break;
    case WpaConnection::STATE_CONNECTING:
        state_widget_.setState(tr("Connecting..."));
        break;
    case WpaConnection::STATE_CONNECTED:
    case WpaConnection::STATE_ACQUIRING_ADDRESS:
        state_widget_.setState(tr("Connected. Acquiring address..."));
        break;
    case WpaConnection::STATE_ACQUIRING_ADDRESS_ERROR:
        state_widget_.setState(tr("Could not acquire address."));
        break;
    case WpaConnection::STATE_COMPLETE:
        {
            QString text(tr("Connected: %1"));
            text = text.arg(proxy_.address());
            state_widget_.setState(text);
            QTimer::singleShot(0, this, SLOT(onComplete()));
        }
        break;
    case WpaConnection::STATE_CONNECT_ERROR:
    case WpaConnection::STATE_DISCONNECT:
        state_widget_.setState(tr("Not connected."));
        break;
    case WpaConnection::STATE_TIMEOUT:
        state_widget_.setState(tr("Timeout."));
        break;
    case WpaConnection::STATE_ABORTED:
        state_widget_.setState(tr("Aborted."));
    default:
        break;
    }
}

/// Authentication handler. When the AP needs psk, we try to check
/// if the password is stored in database or not. If possible, we
/// can use the password remembered.
void WifiDialog::onNeedPassword(WifiProfile profile)
{
    if (ap_dialog_visible_)
    {
        return;
    }
    qDebug("Need password now, password incorrect or not available.");

    // No password remembered or incorrect.
    bool ok = showConfigurationDialog(profile);
    if (ok)
    {
        // We can store the AP here as user already updated password.
        storeAp(profile);

        // Connect again.
        proxy_.connectTo(profile);
    }
    else
    {
        proxy_.stop();
        updateStateLable(WpaConnection::STATE_ABORTED);
    }
}

void WifiDialog::onNoMatchedAP()
{
    proxy_.scanResults(scan_results_);
    arrangeAPItems(scan_results_);
}

void WifiDialog::onComplete()
{
    accept();
}

void WifiDialog::onItemActivated(CatalogView *catalog, ContentView *item, int user_data)
{
    if (!item || !item->data())
    {
        return;
    }
    WifiProfile * d = static_cast<WifiProfile *>(item->data());
    static_cast<WifiAPItem *>(item)->activateItem();
    onAPItemClicked(*d);
}

void WifiDialog::onPositionChanged(const int, const int)
{
    showPaginationButtons(ap_view_.hasPrev(), ap_view_.hasNext());
}

void WifiDialog::onAPConfig(WifiProfile &profile)
{
    if (showConfigurationDialog(profile))
    {
        onAPItemClicked(profile);
    }
}

void WifiDialog::setPassword(WifiProfile & profile,
                             const QString & password)
{
    if (profile.isWpa() || profile.isWpa2())
    {
        profile.setPsk(password);
    }
    else if (profile.isWep())
    {
        profile.setWepKey1(password);
    }
}

/// Store access point that successfully connected.
void WifiDialog::storeAp(WifiProfile & profile)
{
    sys::SystemConfig conf;
    WifiProfiles all = records(conf);

    profile.increaseCount();
    for(int i = 0; i < all.size(); ++i)
    {
        if (all[i].bssid() == profile.bssid())
        {
            profile.resetRetry();
            all.replace(i, profile);
            conf.saveWifiProfiles(all);
            return;
        }
    }

    all.push_front(profile);
    conf.saveWifiProfiles(all);
}

WifiProfiles WifiDialog::records(sys::SystemConfig &conf)
{
    WifiProfiles records;
    conf.loadWifiProfiles(records);
    return records;
}

void WifiDialog::updateHardwareAddress()
{
    QString text(tr("Hardware Address: %1"));
    text = text.arg(proxy_.hardwareAddress());
    hardware_address_.setText(text);
}

bool WifiDialog::showConfigurationDialog(WifiProfile &profile)
{
    ap_dialog_visible_ = true;
    ApConfigDialog dialog(this, profile);
    int ret = dialog.popup();
    ap_dialog_visible_ = false;

    // Update screen.
    update();
    onyx::screen::watcher().enqueue(this, onyx::screen::ScreenProxy::GC);

    // Check return value.
    if (ret == QDialog::Accepted)
    {
        return true;
    }
    return false;
}

void WifiDialog::showPaginationButtons(bool show_prev,
                                          bool show_next)
{
    prev_button_.setVisible(show_prev);
    next_button_.setVisible(show_next);
}

}   // namespace ui

///
/// \example wifi/wifi_gui_test.cpp
/// This is an example of how to use the WifiDialog class.
///
