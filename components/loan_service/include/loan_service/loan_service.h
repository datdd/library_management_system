#ifndef LMS_LOAN_SERVICE_LOAN_SERVICE_H
#define LMS_LOAN_SERVICE_LOAN_SERVICE_H

#include "catalog_service/i_catalog_service.h"
#include "i_loan_service.h"
#include "notification_service/i_notification_service.h"
#include "persistence_service/i_persistence_service.h"
#include "user_service/i_user_service.h"
#include "utils/date_time_utils.h"  // For date calculations

#include <memory>  // For std::shared_ptr
#include <string>  // For std::to_string for generating IDs
#include <mutex>   // For std::mutex

namespace lms {
namespace loan_service {

class LoanService : public ILoanService {
public:
  LoanService(
      std::shared_ptr<catalog_service::ICatalogService> catalog_service,
      std::shared_ptr<user_service::IUserService> user_service,
      std::shared_ptr<persistence_service::IPersistenceService> persistence_service,
      std::shared_ptr<notification_service::INotificationService> notification_service,
      std::shared_ptr<utils::DateTimeUtils> date_time_utils,  // Pass as dependency for testability
      int default_loan_duration_days = 14                     // Configurable loan duration
  );

  ~LoanService() override = default;

  LoanRecord borrowItem(const EntityId& user_id, const EntityId& item_id) override;
  void returnItem(const EntityId& user_id, const EntityId& item_id) override;
  std::vector<LoanRecord> getActiveLoansForUser(const EntityId& user_id) const override;
  std::vector<LoanRecord> getLoanHistoryForUser(const EntityId& user_id) const override;
  std::vector<LoanRecord> getLoanHistoryForItem(const EntityId& item_id) const override;
  void processOverdueItems() override;

private:
  std::shared_ptr<catalog_service::ICatalogService> m_catalog_service;
  std::shared_ptr<user_service::IUserService> m_user_service;
  std::shared_ptr<persistence_service::IPersistenceService> m_persistence_service;
  std::shared_ptr<notification_service::INotificationService> m_notification_service;
  std::shared_ptr<utils::DateTimeUtils> m_date_time_utils;
  int m_default_loan_duration_days;

  // Helper to generate unique loan record IDs (simple approach)
  EntityId generateLoanRecordId() const;
  mutable long long m_next_loan_id_counter = 0;  // For generateLoanRecordId
  mutable std::mutex m_id_mutex;                 // For thread-safe ID generation
};

}  // namespace loan_service
}  // namespace lms

#endif  // LMS_LOAN_SERVICE_LOAN_SERVICE_H