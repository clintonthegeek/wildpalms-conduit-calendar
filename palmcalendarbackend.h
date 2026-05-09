#ifndef WILDPALMS_CALENDAR_PALMCALENDARBACKEND_H
#define WILDPALMS_CALENDAR_PALMCALENDARBACKEND_H

#include "syncbackend.h"

#include <QObject>

namespace WildPalms::PalmSync { class PalmBackend; }
namespace WildPalms::PalmCalendar { class CategoryMappingStore; }

namespace WildPalms::CalendarPlugin {

class PalmCalendarBackend final : public Kalburator::Sync::SyncBackend
{
    Q_OBJECT
public:
    static constexpr const char *BackendId        = "calendar";
    static constexpr const char *PalmDbName       = "DatebookDB";
    static constexpr const char *CollectionPrefix = "palm:calendar/";

    explicit PalmCalendarBackend(
        WildPalms::PalmSync::PalmBackend *palmBackend,
        const WildPalms::PalmCalendar::CategoryMappingStore *categoryStore,
        QObject *parent = nullptr);
    ~PalmCalendarBackend() override;

    // SyncBackend identity
    QString backendType() const override { return QStringLiteral("palm-calendar"); }
    QList<Kalburator::Shape::Shape> nativeShapes() const override;

    // IBlobBackend identity
    QString backendId()   const override;
    QString displayName() const override;
    bool    isAvailable() const override;

    // IBlobBackend collections
    QList<Kalburator::Sync::CollectionInfo> availableCollections() override;
    Kalburator::Sync::CollectionInfo collectionInfo(const QString &collectionId) override;
    QString createCollection(const Kalburator::Sync::CollectionInfo &info) override;

    // IBlobBackend records
    QList<Kalburator::Sync::BackendRecord> loadRecords(const QString &collectionId) override;
    std::optional<Kalburator::Sync::BackendRecord> loadRecord(const QString &recordId) override;
    QString createRecord(const QString &collectionId,
                         const Kalburator::Sync::BackendRecord &record) override;
    bool    updateRecord(const Kalburator::Sync::BackendRecord &record) override;
    bool    deleteRecord(const QString &recordId) override;

    // IBlobBackend change detection
    QList<Kalburator::Sync::BackendRecord> modifiedSince(
        const QString &collectionId, const QDateTime &since) override;
    QStringList deletedSince(const QString &collectionId, const QDateTime &since) override;
    bool        supportsDeleteTracking() const override;

    // Helpers (exposed for tests)
    static int     slotFromCollectionId(const QString &collectionId);
    static QString collectionIdForSlot(int slot);

    // SyncBackend calendar pure-virtuals — stubs; dispatchBlobSync never calls these
    void loadCalendars(const QString &) override {}
    void storeCalendars(const QString &,
                        const QList<KCalendarCore::MemoryCalendar *> &) override {}
    void startSync(const QString &,
                   KCalendarCore::MemoryCalendar *,
                   const QList<KCalendarCore::Incidence::Ptr> &,
                   const QList<KCalendarCore::Incidence::Ptr> &,
                   const QMap<QString, QString> &,
                   const Kalburator::Sync::TranscodingPlan &) override {}
    void removeItem(const QString &, const QString &) override {}
    Kalburator::Sync::PushOperation *pushItems(
        const QString &,
        const QList<KCalendarCore::Incidence::Ptr> &,
        const Kalburator::Sync::TranscodingPlan &) override { return nullptr; }

Q_SIGNALS:
    void recordCreated(const QString &recordId);
    void recordUpdated(const QString &recordId);
    void recordDeleted(const QString &recordId);
    void errorOccurred(const QString &error);
    void progressUpdated(int current, int total, const QString &message);

private:
    WildPalms::PalmSync::PalmBackend                    *m_palmBackend   = nullptr;
    const WildPalms::PalmCalendar::CategoryMappingStore *m_categoryStore = nullptr;
};

} // namespace WildPalms::CalendarPlugin

#endif // WILDPALMS_CALENDAR_PALMCALENDARBACKEND_H
