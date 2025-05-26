Integrating MS SQL Server

**Prerequisites & Considerations:**

1.  **MS SQL Server Instance:** You need access to an MS SQL Server instance (local, remote, Docker, Azure SQL, etc.).
2.  **Database and Schema:** You'll need to create a database and define tables for your entities (Authors, Books, Users, LoanRecords).
3.  **C++ SQL Connector/Driver:** You need a library to connect to and interact with MS SQL Server from C++. Common choices include:
    *   **ODBC (Open Database Connectivity):** This is a standard API. You'd use the "SQL Server Native Client" ODBC driver or the newer "Microsoft ODBC Driver for SQL Server". This is often a good cross-platform starting point if you want to be somewhat database-agnostic later (though SQL syntax varies).
    *   **OLE DB:** More Windows-specific, COM-based.
    *   **SqlClient (from .NET, via C++/CLI):** If you're in a mixed-mode environment, but generally not for pure standard C++.
    *   **Third-party C++ Libraries:** Some libraries wrap ODBC or provide their own native connectors (e.g., `SQLAPI++`, `soci`, `odb`). These can simplify development but add another dependency.

    For this example, we'll conceptualize using **ODBC** as it's a widely available and standard approach.

4.  **Connection String:** You'll need a connection string to connect to your SQL Server database.

**Step 0: Install SQL Server on Linux**

Pull and run the SQL Server Linux container image
```PowerShell
docker run -e "ACCEPT_EULA=Y" -e "MSSQL_SA_PASSWORD=<password>" -p 1433:1433 --name sql1 --hostname sql1 -d mcr.microsoft.com/mssql/server:2022-latest
```

[Install document](https://learn.microsoft.com/en-us/sql/linux/quickstart-install-connect-docker?view=sql-server-ver16&tabs=cli&pivots=cs1-powershell)

**Database Schema Design**

Before writing C++ code, define your SQL Server table structures.

*   **Authors Table:**
    ```sql
    CREATE TABLE Authors (
        AuthorId NVARCHAR(255) PRIMARY KEY,
        Name NVARCHAR(255) NOT NULL
    );
    ```
*   **Users Table:**
    ```sql
    CREATE TABLE Users (
        UserId NVARCHAR(255) PRIMARY KEY,
        Name NVARCHAR(255) NOT NULL
    );
    ```
*   **LibraryItems Table (for Books):**
    ```sql
    CREATE TABLE LibraryItems (
        ItemId NVARCHAR(255) PRIMARY KEY,
        ItemType NVARCHAR(50) NOT NULL DEFAULT 'Book', -- For future different item types
        Title NVARCHAR(512) NOT NULL,
        AuthorId NVARCHAR(255) NULL, -- Can be NULL if item type doesn't have an author
        ISBN NVARCHAR(50) NULL,      -- Specific to Book
        PublicationYear INT NULL,    -- Specific to Book
        AvailabilityStatus INT NOT NULL, -- 0: Available, 1: Borrowed, etc.
        FOREIGN KEY (AuthorId) REFERENCES Authors(AuthorId) ON DELETE SET NULL -- Or ON DELETE NO ACTION
    );
    ```
*   **LoanRecords Table:**
    ```sql
    CREATE TABLE LoanRecords (
        LoanRecordId NVARCHAR(255) PRIMARY KEY,
        ItemId NVARCHAR(255) NOT NULL,
        UserId NVARCHAR(255) NOT NULL,
        LoanDate DATETIME2 NOT NULL,
        DueDate DATETIME2 NOT NULL,
        ReturnDate DATETIME2 NULL,
        FOREIGN KEY (ItemId) REFERENCES LibraryItems(ItemId) ON DELETE CASCADE, -- Or NO ACTION
        FOREIGN KEY (UserId) REFERENCES Users(UserId) ON DELETE CASCADE       -- Or NO ACTION
    );
    ```