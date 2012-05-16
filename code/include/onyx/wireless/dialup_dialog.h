
/// Dial up based configuration dialog.
#ifndef UI_DIALUP_DIALOG_H_
#define UI_DIALUP_DIALOG_H_

#include <map>
#include <QtGui/QtGui>
#include <QHostInfo>
#include "ap_item.h"
#include "onyx/ui/catalog_view.h"

using namespace sys;

namespace ui
{


class DialUpDialog : public QDialog
{
    Q_OBJECT

public:
    DialUpDialog(QWidget *parent, SysStatus & sys);
    ~DialUpDialog();

public:
    int  popup(bool show_profile = true);
    void connect(const QString & file, const QString & username, const QString &password);

protected:
    virtual void keyPressEvent(QKeyEvent *);
    virtual void keyReleaseEvent(QKeyEvent *);
    virtual bool event(QEvent * event);
    virtual void paintEvent(QPaintEvent *e);
    virtual void resizeEvent(QResizeEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);

private Q_SLOTS:
    void onTimeout();
    void onCloseClicked();
    void onConnectClicked(bool);
    void onGetFocus(OnyxLineEdit *object);
    void onPppConnectionChanged(const QString &, int);
    void showDNSResult(QHostInfo);

    void onDisconnectClicked(void);

    void onItemActivated(CatalogView*, ContentView*, int);

    void onReport3GNetwork(const int signal, const int total, const int network);
    void showOffMessage();

    void onDialogAccept();

private:
    void createLayout();
    void clear();
    void createAPNsButtons();
    void createDisconnectButton();
    void updateStatus(QString status);

    bool isConnecting() { return connecting_; }
    void setConnecting(bool c) { connecting_ = c; }

    void loadConf();
    void saveConf();

private:
    QVBoxLayout  big_box_;
    QWidget title_widget_;

    QVBoxLayout title_vbox_;
    QHBoxLayout  title_hbox_;
    QVBoxLayout content_layout_;

    QHBoxLayout state_box_;
    OnyxLabel network_label_;
    OnyxLabel state_widget_;

    QGridLayout input_layout_;
    // OnyxLabel number_label_;
    // OnyxLineEdit number_edit_;

    QFrame top_label_;
    OnyxLabel title_icon_label_;
    OnyxLabel title_text_label_;
    OnyxPushButton close_button_;
   
    ui::CatalogView APNS_buttons_;
    ui::CatalogView disconnect_button_;
    ODatas apns_buttons_datas_;
    OData* selected_;

    SysStatus & sys_;
    QTimer timer_;
    bool connecting_;

    DialupProfiles all_peers_;
    DialupProfile profile_;
};

}

#endif // UI_WIFI_DIALOG_H_
