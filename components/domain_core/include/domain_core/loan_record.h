#ifndef LMS_DOMAIN_CORE_LOAN_RECORD_H
#define LMS_DOMAIN_CORE_LOAN_RECORD_H

#include "types.h" // Should include <optional> and define Date
#include <optional> // Explicitly include if types.h doesn't pull it transitively for Date

namespace lms {
namespace domain_core {

class LoanRecord {
public:
    LoanRecord(EntityId record_id, EntityId item_id, EntityId user_id, Date loan_date, Date due_date);

    const EntityId& getRecordId() const;
    const EntityId& getItemId() const;
    const EntityId& getUserId() const;
    const Date& getLoanDate() const;
    const Date& getDueDate() const;
    void setDueDate(Date due_date);

    // Ensure this is the signature:
    const std::optional<Date>& getReturnDate() const;
    void setReturnDate(Date return_date);

    bool operator==(const LoanRecord& other) const;
    bool operator!=(const LoanRecord& other) const;

private:
    EntityId m_record_id;
    EntityId m_item_id;
    EntityId m_user_id;
    Date m_loan_date;
    Date m_due_date;
    std::optional<Date> m_return_date;
};

} // namespace domain_core
} // namespace lms

#endif // LMS_DOMAIN_CORE_LOAN_RECORD_H