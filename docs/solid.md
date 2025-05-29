# S.O.L.I.D. Principles Review

## 1. Single Responsibility Principle (SRP)

**Concept:**  
A class or module should have only one reason to change, meaning it should have a single primary responsibility.

**How it's applied in LMS:**  
- **Domain Core (`domain_core`):**
  - **Entities:**  
    - `Author`, `User`, `Book`, `LoanRecord`, `ILibraryItem`  
    - Each class is responsible for managing its specific entity’s data and invariants (e.g., the `User` constructor validates the ID and name, and `Book` manages book-specific attributes).
  - **Interface:**  
    - `ILibraryItem` defines the contract that all library items must follow.
- **Service Layers:**
  - `UserService`: Responsible for user management (add, find, update users) without concern for data storage.
  - `CatalogService`: Manages library items and author-related data while delegating persistence details to the persistence layer.
  - `LoanService`: Handles the business logic of borrowing and returning items, orchestrating interactions between users, items, and persistence.
  - `NotificationService` (and its implementation `ConsoleNotificationService`): Solely responsible for sending notifications.
- **Persistence Services:**
  - Implementations such as `InMemoryPersistenceService`, `FilePersistenceService`, `MsSqlPersistenceService`, and `CachingFilePersistenceService` each handle a specific storage mechanism while adhering to the `IPersistenceService` interface.
- **Utilities:**
  - `DateTimeUtils` is dedicated to date and time manipulation functions.
- **Application (`app`):**
  - `main.cc` sets up the application by initializing services and managing command-line interactions.

**Benefits:**  
- **Reduced Coupling:** Changes to one module (e.g., how users are stored) affect only that module.
- **Easier Maintenance:** Focused responsibilities make the logic easier to understand and test.
- **Improved Testability:** Smaller, self-contained classes facilitate isolated unit testing.

## 2. Open/Closed Principle (OCP)

**Concept:**  
Software entities should be open for extension but closed for modification.

**How it's applied in LMS:**  
- **Persistence:**  
  - The `IPersistenceService` interface allows you to add new persistence mechanisms (e.g., `FilePersistenceService`, `MsSqlPersistenceService`) without changing the existing services that use persistence.
- **Notification:**  
  - New notification methods (like `EmailNotificationService` or `SmsNotificationService`) can be introduced by implementing the `INotificationService` interface, keeping existing code unaffected.
- **Library Items:**  
  - The `ILibraryItem` interface supports extending the types of library items (e.g., adding `Magazine` or `DVD`) without modifying the consuming services.

**Benefits:**  
- **Stability:** New features can be added without risking regression in existing functionality.
- **Flexibility:** The core system remains unchanged even if additional behaviors or storage methods are introduced.

## 3. Liskov Substitution Principle (LSP)

**Concept:**  
Subtypes must be substitutable for their base types without altering the correctness of the program.

**How it's applied in LMS:**  
- **Library Items:**  
  - Services expecting an `ILibraryItem` (or a `std::unique_ptr<ILibraryItem>`) can work with any conforming subtype (e.g., `Book`, or a future `Magazine`).
- **Persistence Implementations:**  
  - Clients can use any implementation of `IPersistenceService` (like `FilePersistenceService` or `InMemoryPersistenceService`) interchangeably.
- **Notification Services:**  
  - Implementations such as `ConsoleNotificationService` adhere to `INotificationService` ensuring that they can be substituted without affecting client code.

**Benefits:**  
- **Reliability:** Guarantees that substituting implementations does not break the program.
- **Maintainability:** Supports safe extension and replacement of components.

## 4. Interface Segregation Principle (ISP)

**Concept:**  
Clients should not be forced to depend on interfaces they do not use. It's preferable to have many small, specific interfaces over one large, general-purpose interface.

**How it's applied in LMS:**  
- The system is divided into focused interfaces:
  - `IUserService` for user operations.
  - `ICatalogService` for managing library items.
  - `ILoanService` for loan activities.
  - `IPersistenceService` for data storage operations.
  - `INotificationService` for sending notifications.
- For instance, `UserService` only depends on the user-related methods exposed by `IPersistenceService`, without being coupled to methods for catalog management.

**Benefits:**  
- **Reduced Coupling:** Smaller interfaces limit the impact of changes.
- **Easier Implementation:** Focused responsibilities reduce complexity and improve code clarity.

## 5. Dependency Inversion Principle (DIP)

**Concept:**  
High-level modules should not depend on low-level modules; both should depend on abstractions. Details should depend on abstractions, not vice versa.

**How it's applied in LMS:**  
- **High-Level Modules:**  
  - `UserService`, `CatalogService`, and `LoanService` depend on abstractions (e.g., `IPersistenceService`, `INotificationService`) rather than on concrete implementations.
- **Dependency Injection:**  
  - Concrete implementations are injected into high-level modules at the composition root (often in `app/main.cc`), allowing for easy swapping of implementations.
- **Benefits:**  
  - **Decoupling:** High-level modules are insulated from changes in low-level details.
  - **Testability:** Mock implementations can be injected to facilitate unit testing.

## Summary of SOLID Compliance

- **SRP:** Classes and services have clear, singular responsibilities.
- **OCP:** Core functionality remains stable while allowing extension through new implementations.
- **LSP:** Subtypes are interchangeable, ensuring robust and reliable behavior.
- **ISP:** Clients use only the interfaces that are relevant to them, reducing unnecessary dependencies.
- **DIP:** High-level modules rely on abstractions, improving flexibility, testability, and maintainability.

This design leads to a modular, extensible, and testable system, aligning well with the architectural goals of the library management system.