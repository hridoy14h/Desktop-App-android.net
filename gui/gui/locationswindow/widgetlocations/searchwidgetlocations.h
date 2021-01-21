#ifndef SEARCHWIDGETLOCATIONS_H
#define SEARCHWIDGETLOCATIONS_H

#include <QElapsedTimer>
#include "customscrollbar.h"
#include <QGraphicsScene>
#include <QTimer>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QEasingCurve>
#include "locationitem.h"
#include "../backend/locationsmodel/basiclocationsmodel.h"
#include "../backend/locationsmodel/favoritelocationsstorage.h"
#include "../backend/types/types.h"
#include "iwidgetlocationsinfo.h"
#include "backgroundpixmapanimation.h"
#include "tooltips/tooltiptypes.h"
#include "locationitemlistwidget.h"
#include "scrollbar.h"


class FormConnect;

namespace GuiLocations {

// TODO: test scaling changes in all contained classes
// TODO: test against account that loses/gains pro status
// TODO: test against disabled servers
// TODO: test keypress navigation with cursor when accessibility permission is off

// TODO: add fast scrolling when holding down/up (only when connect window has focus)
class SearchWidgetLocations : public QScrollArea, public IWidgetLocationsInfo
{
    Q_OBJECT

public:
    explicit SearchWidgetLocations(QWidget *parent);
    ~SearchWidgetLocations() override;

    void setFilterString(QString text);

    bool cursorInViewport() override;
    bool hasSelection() override;
    void centerCursorOnSelectedItem() override;

    void setModel(BasicLocationsModel *locationsModel);
    void setFirstSelected() override;
    void startAnimationWithPixmap(const QPixmap &pixmap);

    void setShowLatencyInMs(bool showLatencyInMs);
    virtual bool isShowLatencyInMs()   override;
    virtual bool isFreeSessionStatus() override;

    virtual int getWidth()          override;
    virtual int getScrollBarWidth() override;

    void setCountAvailableItemSlots(int cnt);

    bool eventFilter(QObject *object, QEvent *event) override;
    void handleKeyEvent(QKeyEvent *event) override;

    int countVisibleItems() override; // visible is ambiguous

    void updateScaling();

protected:
    virtual void paintEvent(QPaintEvent *event)            override;
    virtual void scrollContentsBy(int dx, int dy)          override;
    virtual void mouseMoveEvent(QMouseEvent *event)        override;
    virtual void mousePressEvent(QMouseEvent *event)       override;
    virtual void mouseReleaseEvent(QMouseEvent *event)     override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
    virtual void leaveEvent(QEvent *event)                 override;
    virtual void enterEvent(QEvent *event)                 override;
    virtual void resizeEvent(QResizeEvent *event)          override;

signals:
    void selected(LocationID id);
    void switchFavorite(LocationID id, bool isFavorite);
    void addStaticIpURLClicked();

private slots:
    void onItemsUpdated(QVector<LocationModelItem*> items);
    void onConnectionSpeedChanged(LocationID id, PingTime timeMs);
    void onIsFavoriteChanged(LocationID id, bool isFavorite);
    void onFreeSessionStatusChanged(bool isFreeSessionStatus);

    void onLanguageChanged();

    void onLocationItemListWidgetHeightChanged(int listWidgetHeight);
    void onLocationItemListWidgetFavoriteClicked(LocationItemCityWidget *cityWidget, bool favorited);
    void onLocationItemListWidgetLocationIdSelected(LocationID id);
    void onLocationItemListWidgetRegionExpanding(LocationItemRegionWidget *region);

    void onScrollAnimationValueChanged(const QVariant &value);
private:
    LocationItemListWidget *locationItemListWidget_;
    ScrollBar *scrollBar_;
    QVariantAnimation scrollAnimation_;

    QString filterString_;
    int countOfAvailableItemSlots_;
    bool bIsFreeSession_;
    bool bShowLatencyInMs_;
    bool bTapGestureStarted_;
    BasicLocationsModel *locationsModel_;
    BackgroundPixmapAnimation backgroundPixmapAnimation_;

    void updateWidgetList(QVector<LocationModelItem *> items);

    // still used?
    int getItemHeight() const;
    int getTopOffset() const;
    bool isGlobalPointInViewport(const QPoint &pt);
    void handleTapClick(const QPoint &cursorPos);
    QRect globalLocationsListViewportRect();

    // unused -- maybe useful
    bool isExpandAnimationNow();
    void setCursorForSelected();
    void setVisibleExpandedItem(int ind);
    LocationID detectLocationForTopInd(int topInd);
    int detectVisibleIndForCursorPos(const QPoint &pt);

    // new
    const QString scrollbarStyleSheet();
    void scrollDown(int itemCount);
    void animatedScrollDown(int itemCount);
    void animatedScrollUp(int itemCount);
    int animationScollTarget_;
};

} // namespace GuiLocations


#endif // SEARCHWIDGETLOCATIONS_H
