#include "preferenceswindowitem.h"

#include <QPainter>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QCoreApplication>

#include "graphicresources/imageresourcessvg.h"
#include "preferencestab/preferencestabcontrolitem.h"
#include "graphicresources/fontmanager.h"
#include "languagecontroller.h"
#include "utils/ws_assert.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {


PreferencesWindowItem::PreferencesWindowItem(QGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper, AccountInfo *accountInfo)
    : IPreferencesWindow(parent, preferences, preferencesHelper), isShowSubPage_(false), loggedIn_(false)
{
    setFlags(QGraphicsObject::ItemIsFocusable);
    setMinimumHeight(kMinHeight);
    installEventFilter(this);

    tabControlItem_ = new PreferencesTabControlItem(this, preferencesHelper);
    connect(dynamic_cast<QObject*>(tabControlItem_), SIGNAL(currentTabChanged(PREFERENCES_TAB_TYPE)), SLOT(onCurrentTabChanged(PREFERENCES_TAB_TYPE)));
    connect(dynamic_cast<QObject*>(tabControlItem_), SIGNAL(signOutClick()), SIGNAL(signOutClick()));
    connect(dynamic_cast<QObject*>(tabControlItem_), SIGNAL(loginClick()), SIGNAL(loginClick()));
    connect(dynamic_cast<QObject*>(tabControlItem_), SIGNAL(quitClick()), SIGNAL(quitAppClick()));

    scrollAreaItem_ = new CommonGraphics::ScrollArea(this, curHeight_ - 102*G_SCALE, WINDOW_WIDTH - kTabAreaWidth);

    generalWindowItem_ = new GeneralWindowItem(nullptr, preferences, preferencesHelper);

    accountWindowItem_ = new AccountWindowItem(nullptr, accountInfo);
    accountWindowItem_->setLoggedIn(false);
    connect(accountWindowItem_, &AccountWindowItem::sendConfirmEmailClick, this, &PreferencesWindowItem::sendConfirmEmailClick);
    connect(accountWindowItem_, &AccountWindowItem::accountLoginClick, this, &PreferencesWindowItem::accountLoginClick);
    connect(accountWindowItem_, &AccountWindowItem::manageAccountClick, this, &PreferencesWindowItem::manageAccountClick);
    connect(accountWindowItem_, &AccountWindowItem::addEmailButtonClick, this, &PreferencesWindowItem::addEmailButtonClick);

    connectionWindowItem_ = new ConnectionWindowItem(nullptr, preferences, preferencesHelper);
    connect(connectionWindowItem_, &ConnectionWindowItem::networkOptionsPageClick, this, &PreferencesWindowItem::onNetworkOptionsPageClick);
    connect(connectionWindowItem_, &ConnectionWindowItem::splitTunnelingPageClick, this, &PreferencesWindowItem::onSplitTunnelingPageClick);
    connect(connectionWindowItem_, &ConnectionWindowItem::proxySettingsPageClick, this, &PreferencesWindowItem::onProxySettingsPageClick);
    connect(connectionWindowItem_, &ConnectionWindowItem::cycleMacAddressClick, this, &PreferencesWindowItem::cycleMacAddressClick);
    connect(connectionWindowItem_, &ConnectionWindowItem::detectPacketSize, this, &PreferencesWindowItem::detectPacketSizeClick);

    advancedWindowItem_ = new AdvancedWindowItem(nullptr, preferences, preferencesHelper);
    connect(advancedWindowItem_, &AdvancedWindowItem::advParametersClick, this, &PreferencesWindowItem::onAdvParametersClick);

#ifdef Q_OS_WIN
    connect(advancedWindowItem_, &AdvancedWindowItem::setIpv6StateInOS, this, &PreferencesWindowItem::setIpv6StateInOS);
#endif

    robertWindowItem_ = new RobertWindowItem(nullptr, preferences, preferencesHelper);
    connect(robertWindowItem_, &RobertWindowItem::accountLoginClick, this, &PreferencesWindowItem::accountLoginClick);
    connect(robertWindowItem_, &RobertWindowItem::manageRobertRulesClick, this, &PreferencesWindowItem::manageRobertRulesClick);
    connect(robertWindowItem_, &RobertWindowItem::setRobertFilter, this, &PreferencesWindowItem::setRobertFilter);
    helpWindowItem_ = new HelpWindowItem(nullptr, preferences, preferencesHelper);
    connect(helpWindowItem_, &HelpWindowItem::viewLogClick, this, &PreferencesWindowItem::viewLogClick);
    connect(helpWindowItem_, &HelpWindowItem::sendLogClick, this, &PreferencesWindowItem::sendDebugLogClick);
    aboutWindowItem_ = new AboutWindowItem(nullptr, preferences, preferencesHelper);

    scrollAreaItem_->setItem(generalWindowItem_);

    networkOptionsWindowItem_ = new NetworkOptionsWindowItem(nullptr, preferences);
    networkOptionsNetworkWindowItem_ = new NetworkOptionsNetworkWindowItem(nullptr, preferences, preferencesHelper);
    proxySettingsWindowItem_ = new ProxySettingsWindowItem(nullptr, preferences);
    splitTunnelingWindowItem_ = new SplitTunnelingWindowItem(nullptr, preferences);
    splitTunnelingAppsWindowItem_ = new SplitTunnelingAppsWindowItem(nullptr, preferences);
    splitTunnelingAddressesWindowItem_ = new SplitTunnelingAddressesWindowItem(nullptr, preferences);

    connect(splitTunnelingWindowItem_, &SplitTunnelingWindowItem::appsPageClick, this, &PreferencesWindowItem::onSplitTunnelingAppsClick);
    connect(splitTunnelingWindowItem_, &SplitTunnelingWindowItem::addressesPageClick, this, &PreferencesWindowItem::onSplitTunnelingAddressesClick);
    connect(splitTunnelingAppsWindowItem_, &SplitTunnelingAppsWindowItem::addButtonClicked, this, &PreferencesWindowItem::splitTunnelingAppsAddButtonClick);
    connect(splitTunnelingAppsWindowItem_, &SplitTunnelingAppsWindowItem::appsUpdated, this, &PreferencesWindowItem::onAppsWindowAppsUpdated);
    connect(splitTunnelingAppsWindowItem_, &SplitTunnelingAppsWindowItem::escape, this, &PreferencesWindowItem::onStAppsEscape);

    connect(splitTunnelingAddressesWindowItem_, &SplitTunnelingAddressesWindowItem::addressesUpdated, this, &PreferencesWindowItem::onAddressesUpdated);
    connect(splitTunnelingAddressesWindowItem_, &SplitTunnelingAddressesWindowItem::escape, this, &PreferencesWindowItem::onIpsAndHostnameEscape);

    connect(networkOptionsWindowItem_, &NetworkOptionsWindowItem::currentNetworkUpdated, this, &PreferencesWindowItem::onCurrentNetworkUpdated);
    connect(networkOptionsWindowItem_, &NetworkOptionsWindowItem::networkClicked, this, &PreferencesWindowItem::onNetworkOptionsNetworkClick);
    connect(networkOptionsNetworkWindowItem_, &NetworkOptionsNetworkWindowItem::escape, this, &PreferencesWindowItem::onNetworkEscape);
}

PreferencesWindowItem::~PreferencesWindowItem()
{
    delete networkOptionsWindowItem_;
    delete proxySettingsWindowItem_;
    delete splitTunnelingWindowItem_;
    delete splitTunnelingAppsWindowItem_;
    delete splitTunnelingAddressesWindowItem_;

    delete generalWindowItem_;
    delete accountWindowItem_;
    delete connectionWindowItem_;
    delete robertWindowItem_;
    delete advancedWindowItem_;
    delete helpWindowItem_;
    delete aboutWindowItem_;
}

void PreferencesWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // resize area background
    qreal initialOpacity = painter->opacity();
    painter->fillRect(boundingRect().adjusted(0, 286*G_SCALE, 0, -7*G_SCALE), QBrush(QColor(2, 13, 28)));

    QRect rcCaption;
    // base background
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH)
    {
        QPainterPath path;
#ifdef Q_OS_MAC
        path.addRoundedRect(boundingRect().toRect(), 5*G_SCALE, 5*G_SCALE);
#else
        path.addRect(boundingRect().toRect());
#endif
        painter->setPen(Qt::NoPen);
        painter->fillPath(path, QColor(2, 13, 28));
        painter->setPen(Qt::SolidLine);
        QSharedPointer<IndependentPixmap> pixmapHeader = ImageResourcesSvg::instance().getIndependentPixmap(backgroundHeader_);
        pixmapHeader->draw(0, 0, painter);
        rcCaption = QRect(64*G_SCALE, 2*G_SCALE, 200*G_SCALE, 56*G_SCALE);
    }
    else
    {
        QSharedPointer<IndependentPixmap> pixmapBaseBackground = ImageResourcesSvg::instance().getIndependentPixmap(backgroundBase_);
        pixmapBaseBackground->draw(0, 0, painter);
        QSharedPointer<IndependentPixmap> pixmapHeader = ImageResourcesSvg::instance().getIndependentPixmap(backgroundHeader_);
        pixmapHeader->draw(0, 27*G_SCALE, painter);
        rcCaption = QRect(64*G_SCALE, 30*G_SCALE, 200*G_SCALE, 56*G_SCALE);
    }

    // draw page caption
    //painter->fillRect(rcCaption, QBrush(QColor(255, 45, 61)));
    painter->setPen(Qt::white);
    QFont *font = FontManager::instance().getFont(16, true);
    painter->setFont(*font);
    QFontMetrics fm(*font);
    painter->drawText(rcCaption, Qt::AlignLeft | Qt::AlignVCenter, fm.elidedText(scrollAreaItem_->item()->caption(), Qt::ElideRight, 140*G_SCALE));

    // bottom-most background
    painter->setOpacity(initialOpacity);
    if (roundedFooter_)
    {
        painter->setPen(footerColor_);
        painter->setBrush(footerColor_);
        painter->drawRoundedRect(getBottomResizeArea(), 8*G_SCALE, 8*G_SCALE);
        painter->fillRect(getBottomResizeArea().adjusted(0, -2*G_SCALE, 0, -7*G_SCALE), QBrush(footerColor_));
    }
    else
    {
        painter->fillRect(getBottomResizeArea(), QBrush(footerColor_));
    }
}

void PreferencesWindowItem::setCurrentTab(PREFERENCES_TAB_TYPE tab)
{
    changeTab(tab);
    tabControlItem_->setCurrentTab(tab);
}

void PreferencesWindowItem::setCurrentTab(PREFERENCES_TAB_TYPE tab, CONNECTION_SCREEN_TYPE subpage)
{
    changeTab(tab);

    // set subpage
    if (tab == TAB_CONNECTION)
    {
        if (subpage == CONNECTION_SCREEN_NETWORK_OPTIONS)
        {
            tabControlItem_->setCurrentTab(TAB_CONNECTION);
            connectionWindowItem_->setScreen(CONNECTION_SCREEN_NETWORK_OPTIONS);
            onNetworkOptionsPageClick();
        }
        else if (subpage == CONNECTION_SCREEN_SPLIT_TUNNELING)
        {
            tabControlItem_->setCurrentTab(TAB_CONNECTION);
            connectionWindowItem_->setScreen(CONNECTION_SCREEN_SPLIT_TUNNELING);
            setPreferencesWindowToSplitTunnelingHome();
        }
    }
}

void PreferencesWindowItem::setLoggedIn(bool loggedIn)
{
    tabControlItem_->setLoggedIn(loggedIn);
    accountWindowItem_->setLoggedIn(loggedIn);
    robertWindowItem_->setLoggedIn(loggedIn);
    splitTunnelingAppsWindowItem_->setLoggedIn(loggedIn);
    splitTunnelingAddressesWindowItem_->setLoggedIn(loggedIn);
    loggedIn_ = loggedIn;
}

void PreferencesWindowItem::setConfirmEmailResult(bool bSuccess)
{
    accountWindowItem_->setConfirmEmailResult(bSuccess);
}

void PreferencesWindowItem::setSendLogResult(bool bSuccess)
{
    helpWindowItem_->setSendLogResult(bSuccess);
}

void PreferencesWindowItem::updateNetworkState(types::NetworkInterface network)
{
    networkOptionsWindowItem_->setCurrentNetwork(network);
    networkOptionsNetworkWindowItem_->setCurrentNetwork(network);
    connectionWindowItem_->setCurrentNetwork(network);
}

void PreferencesWindowItem::changeTab(PREFERENCES_TAB_TYPE tab)
{
    if (tab == TAB_GENERAL)
    {
        scrollAreaItem_->setItem(generalWindowItem_);
        generalWindowItem_->updateScaling();
        setShowSubpageMode(false);
        update();
    }
    else if (tab == TAB_ACCOUNT)
    {
        scrollAreaItem_->setItem(accountWindowItem_);
        accountWindowItem_->updateScaling();
        setShowSubpageMode(false);
        update();
    }
    else if (tab == TAB_CONNECTION)
    {
        scrollAreaItem_->setItem(connectionWindowItem_);
        connectionWindowItem_->updateScaling();
        connectionWindowItem_->setScreen(CONNECTION_SCREEN_HOME);
        setShowSubpageMode(false);
        update();
    }
    else if (tab == TAB_ROBERT)
    {
        robertWindowItem_->setError(false);
        if (loggedIn_)
        {
            robertWindowItem_->setLoading(true);
            emit getRobertFilters();
        }
        scrollAreaItem_->setItem(robertWindowItem_);
        robertWindowItem_->updateScaling();
        setShowSubpageMode(false);
        update();
    }
    else if (tab == TAB_ADVANCED)
    {
        scrollAreaItem_->setItem(advancedWindowItem_);
        advancedWindowItem_->updateScaling();
        advancedWindowItem_->setScreen(ADVANCED_SCREEN_HOME);
        setShowSubpageMode(false);
        update();
    }
    else if (tab == TAB_HELP)
    {
        scrollAreaItem_->setItem(helpWindowItem_);
        helpWindowItem_->updateScaling();
        setShowSubpageMode(false);
        update();
    }
    else if (tab == TAB_ABOUT)
    {
        scrollAreaItem_->setItem(aboutWindowItem_);
        aboutWindowItem_->updateScaling();
        setShowSubpageMode(false);
        update();
    }
    else
    {
        WS_ASSERT(false);
    }

    if (tab != TAB_ROBERT) {
        robertWindowItem_->setLoading(false);
    }
}

void PreferencesWindowItem::onCurrentTabChanged(PREFERENCES_TAB_TYPE tab)
{
    changeTab(tab);
}

void PreferencesWindowItem::moveOnePageBack()
{
    PREFERENCES_TAB_TYPE currentTab = tabControlItem_->currentTab();

    if (currentTab == TAB_CONNECTION) {
        CONNECTION_SCREEN_TYPE screen = connectionWindowItem_->getScreen();

        if (screen == CONNECTION_SCREEN_NETWORK_OPTIONS) {
            NETWORK_OPTIONS_SCREEN networkOptionsScreen = networkOptionsWindowItem_->getScreen();
            if (networkOptionsScreen == NETWORK_OPTIONS_HOME) {
                changeTab(tabControlItem_->currentTab());
            } else if (networkOptionsScreen == NETWORK_OPTIONS_DETAILS) {
                onNetworkOptionsPageClick();
            }
        } else if (screen == CONNECTION_SCREEN_SPLIT_TUNNELING) {
            SPLIT_TUNNEL_SCREEN splitTunnelScreen = splitTunnelingWindowItem_->getScreen();

            if (splitTunnelScreen == SPLIT_TUNNEL_SCREEN_HOME) {
                changeTab(tabControlItem_->currentTab());
            } else if (splitTunnelScreen == SPLIT_TUNNEL_SCREEN_APPS) {
                setPreferencesWindowToSplitTunnelingHome();
            } else if (splitTunnelScreen == SPLIT_TUNNEL_SCREEN_APPS_SEARCH) {
                setPreferencesWindowToSplitTunnelingAppsHome();
            } else if (splitTunnelScreen == SPLIT_TUNNEL_SCREEN_IPS_AND_HOSTNAMES) {
                setPreferencesWindowToSplitTunnelingHome();
            }
        } else if (screen == CONNECTION_SCREEN_PROXY_SETTINGS) {
            changeTab(tabControlItem_->currentTab());
        }
    } else { // non-connection screen
        changeTab(tabControlItem_->currentTab());
    }
}

void PreferencesWindowItem::onBackArrowButtonClicked()
{
    if (isShowSubPage_) {
        moveOnePageBack();
    } else {
        emit escape();
    }
}

void PreferencesWindowItem::onNetworkOptionsPageClick()
{
    scrollAreaItem_->setItem(networkOptionsWindowItem_);
    networkOptionsWindowItem_->updateScaling();
    connectionWindowItem_->setScreen(CONNECTION_SCREEN_NETWORK_OPTIONS);
    networkOptionsWindowItem_->setScreen(NETWORK_OPTIONS_HOME);
    setShowSubpageMode(true);
    setFocus();
    update();
}

void PreferencesWindowItem::onNetworkOptionsNetworkClick(types::NetworkInterface network)
{
    scrollAreaItem_->setItem(networkOptionsNetworkWindowItem_);
    networkOptionsNetworkWindowItem_->setNetwork(network);
    networkOptionsNetworkWindowItem_->updateScaling();
    connectionWindowItem_->setScreen(CONNECTION_SCREEN_NETWORK_OPTIONS);
    networkOptionsWindowItem_->setScreen(NETWORK_OPTIONS_DETAILS);
    setShowSubpageMode(true);
    update();
}

void PreferencesWindowItem::setPreferencesWindowToSplitTunnelingHome()
{
    scrollAreaItem_->setItem(splitTunnelingWindowItem_);
    splitTunnelingWindowItem_->updateScaling();
    connectionWindowItem_->setScreen(CONNECTION_SCREEN_SPLIT_TUNNELING);
    splitTunnelingWindowItem_->setScreen(SPLIT_TUNNEL_SCREEN_HOME);
    setShowSubpageMode(true);
    setFocus();
    update();
}

void PreferencesWindowItem::onSplitTunnelingPageClick()
{
    setPreferencesWindowToSplitTunnelingHome();
}

void PreferencesWindowItem::onProxySettingsPageClick()
{
    scrollAreaItem_->setItem(proxySettingsWindowItem_);
    proxySettingsWindowItem_->updateScaling();
    connectionWindowItem_->setScreen(CONNECTION_SCREEN_PROXY_SETTINGS);
    setShowSubpageMode(true);
    setFocus();
    update();
}

void PreferencesWindowItem::onAdvParametersClick()
{
    emit advancedParametersClicked();
}

void PreferencesWindowItem::setPreferencesWindowToSplitTunnelingAppsHome()
{
    scrollAreaItem_->setItem(splitTunnelingAppsWindowItem_);
    splitTunnelingAppsWindowItem_->updateScaling();
    connectionWindowItem_->setScreen(CONNECTION_SCREEN_SPLIT_TUNNELING);
    splitTunnelingWindowItem_->setScreen(SPLIT_TUNNEL_SCREEN_APPS);
    setShowSubpageMode(true);
    update();
}

void PreferencesWindowItem::addApplicationManually(QString filename)
{
    QList<types::SplitTunnelingApp> apps = splitTunnelingAppsWindowItem_->getApps();

    QString friendlyName = Utils::fileNameFromFullPath(filename);

    types::SplitTunnelingApp app;
    app.name = friendlyName;
    app.active = true;
    app.type = SPLIT_TUNNELING_APP_TYPE_USER;
    app.fullName = filename;
    apps.append(app);

    updateSplitTunnelingAppsCount(apps);
    splitTunnelingAppsWindowItem_->addAppManually(app);
}
 
void PreferencesWindowItem::setPacketSizeDetectionState(bool on)
{
    connectionWindowItem_->setPacketSizeDetectionState(on);
}

void PreferencesWindowItem::showPacketSizeDetectionError(const QString &title,
                                                         const QString &message)
{
    connectionWindowItem_->showPacketSizeDetectionError(title, message);
}

void PreferencesWindowItem::onSplitTunnelingAppsClick()
{
    setPreferencesWindowToSplitTunnelingAppsHome();
}

void PreferencesWindowItem::onSplitTunnelingAddressesClick()
{
    scrollAreaItem_->setItem(splitTunnelingAddressesWindowItem_);
    splitTunnelingAddressesWindowItem_->updateScaling();
    connectionWindowItem_->setScreen(CONNECTION_SCREEN_SPLIT_TUNNELING);
    splitTunnelingWindowItem_->setScreen(SPLIT_TUNNEL_SCREEN_IPS_AND_HOSTNAMES);
    setShowSubpageMode(true);
    update();
    splitTunnelingAddressesWindowItem_->setFocusOnTextEntry();
}

void PreferencesWindowItem::updateSplitTunnelingAppsCount(QList<types::SplitTunnelingApp> apps)
{
    int activeApps = 0;
    for (types::SplitTunnelingApp app : qAsConst(apps)) {
        if (app.active) activeApps++;
    }
    splitTunnelingWindowItem_->setAppsCount(activeApps);
}

void PreferencesWindowItem::updatePositions()
{
    bottomResizeItem_->setPos(kBottomResizeOriginX*G_SCALE, curHeight_ - kBottomResizeOffsetY*G_SCALE);
    escapeButton_->setPos(WINDOW_WIDTH*G_SCALE - escapeButton_->boundingRect().width() - 16*G_SCALE, 16*G_SCALE);

    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        tabControlItem_->getGraphicsObject()->setPos(0, 54*G_SCALE);
        backArrowButton_->setPos(16*G_SCALE, 12*G_SCALE);
        scrollAreaItem_->setPos(kTabAreaWidth*G_SCALE, 55*G_SCALE);
        tabControlItem_->setHeight(curHeight_ - 71*G_SCALE);
        scrollAreaItem_->setHeight(curHeight_ - 74*G_SCALE);
    } else {
        tabControlItem_->getGraphicsObject()->setPos(0, 82*G_SCALE);
        backArrowButton_->setPos(16*G_SCALE, 40*G_SCALE);
        scrollAreaItem_->setPos(kTabAreaWidth*G_SCALE, 83*G_SCALE);
        tabControlItem_->setHeight(curHeight_ - 99*G_SCALE);
        scrollAreaItem_->setHeight(curHeight_ - 102*G_SCALE);
    }
}

void PreferencesWindowItem::onAppsWindowAppsUpdated(QList<types::SplitTunnelingApp> apps)
{
    updateSplitTunnelingAppsCount(apps);
}

void PreferencesWindowItem::onAddressesUpdated(QList<types::SplitTunnelingNetworkRoute> routes)
{
    splitTunnelingWindowItem_->setNetworkRoutesCount(routes.count());
}

void PreferencesWindowItem::onStAppsEscape()
{
    setPreferencesWindowToSplitTunnelingHome();
}

void PreferencesWindowItem::onIpsAndHostnameEscape()
{
    setPreferencesWindowToSplitTunnelingHome();
}

void PreferencesWindowItem::onNetworkEscape()
{
    onNetworkOptionsPageClick();
}

void PreferencesWindowItem::onCurrentNetworkUpdated(types::NetworkInterface network)
{
    emit currentNetworkUpdated(network);
}

void PreferencesWindowItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        // if inner item has focus let it handle the [Escape] event
        if (!scrollAreaItem_->hasItemWithFocus()) {
            if (isShowSubPage_) {
                moveOnePageBack();
            } else {
                emit escape();
            }
        }
    } else {
        scrollAreaItem_->handleKeyPressEvent(event);
    }
}

void PreferencesWindowItem::setShowSubpageMode(bool isShowSubPage)
{
    isShowSubPage_ = isShowSubPage;
    tabControlItem_->setInSubpage(isShowSubPage);
}

void PreferencesWindowItem::updateChildItemsAfterHeightChanged()
{
    bottomResizeItem_->setPos(kBottomResizeOriginX*G_SCALE, curHeight_ - kBottomResizeOffsetY*G_SCALE);

    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        tabControlItem_->setHeight(curHeight_ - 71*G_SCALE);
        scrollAreaItem_->setHeight(curHeight_ - 74*G_SCALE);
    } else {
        tabControlItem_->setHeight(curHeight_ - 99*G_SCALE);
        scrollAreaItem_->setHeight(curHeight_ - 102*G_SCALE);
    }
}

void PreferencesWindowItem::setRobertFilters(const QVector<types::RobertFilter> &filters)
{
    robertWindowItem_->setFilters(filters);
}

void PreferencesWindowItem::setRobertFiltersError()
{
    robertWindowItem_->setError(true);
}

void PreferencesWindowItem::setSplitTunnelingActive(bool active)
{
    splitTunnelingWindowItem_->setActive(active);
}

void PreferencesWindowItem::onCollapse()
{
    robertWindowItem_->setLoading(false);
}

} // namespace PreferencesWindow
