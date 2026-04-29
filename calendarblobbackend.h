#ifndef WILDPALMS_CALENDAR_CALENDARBLOBBACKEND_H
#define WILDPALMS_CALENDAR_CALENDARBLOBBACKEND_H

#include "iblobbackend.h"

#include <QObject>

namespace WildPalms::PalmSync { class PalmBackend; }
namespace WildPalms::PalmCalendar { class CategoryMappingStore; }

namespace WildPalms::CalendarPlugin {

/**
 * @brief Transcoding IBlobBackend wrapping PalmBackend's palm:datebook.
 *
 * Surfaces one collection per populated category slot:
 *   - "palm:calendar/0"   "Unfiled" (always present)
 *   - "palm:calendar/<N>" 1..15, present iff
 *     `categoryStore->slotName("DatebookDB", N)` is non-empty.
 *
 * Records route to/from these collections by PalmRecord::category.
 * loadRecords transcodes wire bytes -> iCal bytes via IcsTranscoder;
 * createRecord/updateRecord transcode iCal -> wire and forward to
 * PalmBackend's category-aware createPalmRecord/updatePalmRecord.
 *
 * Lifetime: does NOT own palmBackend or categoryStore. Caller retains
 * ownership; both must outlive the backend.
 */
class CalendarBlobBackend : public QObject, public Kalburator::Sync::IBlobBackend
{
    Q_OBJECT
public:
    static constexpr const char *BackendId        = "calendar";
    static constexpr const char *PalmDbName       = "DatebookDB";
    static constexpr const char *CollectionPrefix = "palm:calendar/";

    explicit CalendarBlobBackend(
        WildPalms::PalmSync::PalmBackend *palmBackend,
        const WildPalms::PalmCalendar::CategoryMappingStore *categoryStore,
        QObject *parent = nullptr);
    ~CalendarBlobBackend() override;

    // --- Identity ---
    QString backendId()   const override;
    QString displayName() const override;
    bool    isAvailable() const override;

    // --- Collections ---
    QList<Kalburator::Sync::CollectionInfo> availableCollections() override;
    Kalburator::Sync::CollectionInfo collectionInfo(const QString &collectionId) override;
    QString createCollection(const Kalburator::Sync::CollectionInfo &info) override;

    // --- Records ---
    QList<Kalburator::Sync::BackendRecord> loadRecords(const QString &collectionId) override;
    std::optional<Kalburator::Sync::BackendRecord> loadRecord(const QString &recordId) override;
    QString createRecord(const QString &collectionId,
                         const Kalburator::Sync::BackendRecord &record) override;
    bool    updateRecord(const Kalburator::Sync::BackendRecord &record) override;
    bool    deleteRecord(const QString &recordId) override;

    // --- Change detection ---
    QList<Kalburator::Sync::BackendRecord> modifiedSince(
        const QString &collectionId, const QDateTime &since) override;
    QStringList deletedSince(const QString &collectionId, const QDateTime &since) override;
    bool        supportsDeleteTracking() const override;

    // --- Helpers (exposed for tests) ---
    /// Parse "palm:calendar/<N>" -> N. Returns -1 on bad input.
    static int slotFromCollectionId(const QString &collectionId);
    /// Produce "palm:calendar/<N>".
    static QString collectionIdForSlot(int slot);

Q_SIGNALS:
    void recordCreated(const QString &recordId);
    void recordUpdated(const QString &recordId);
    void recordDeleted(const QString &recordId);
    void errorOccurred(const QString &error);
    void progressUpdated(int current, int total, const QString &message);

private:
    WildPalms::PalmSync::PalmBackend                     *m_palmBackend = nullptr;
    const WildPalms::PalmCalendar::CategoryMappingStore  *m_categoryStore = nullptr;
};

} // namespace WildPalms::CalendarPlugin

#endif // WILDPALMS_CALENDAR_CALENDARBLOBBACKEND_H
