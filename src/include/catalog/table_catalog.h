//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// table_catalog.h
//
// Identification: src/include/catalog/table_catalog.h
//
// Copyright (c) 2015-17, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// pg_table
//
// Schema: (column: column_name)
// 0: table_id (pkey), 1: table_name, 2: database_id, 3: database_name
//
// Indexes: (index offset: indexed columns)
// 0: table_id
// 1: table_name & database_name
//
//===----------------------------------------------------------------------===//

#pragma once

#include "catalog/abstract_catalog.h"

namespace peloton {
namespace catalog {

class TableCatalog : public AbstractCatalog {
 public:
  // Global Singleton
  static TableCatalog *GetInstance(storage::Database *pg_catalog = nullptr,
                                   type::AbstractPool *pool = nullptr);

  inline oid_t GetNextOid() { return oid_++ | TABLE_OID_MASK; }

  // Write related API
  bool Insert(oid_t table_id, const std::string &table_name, oid_t database_id,
              const std::string &database_name, type::AbstractPool *pool,
              concurrency::Transaction *txn);
  bool DeleteByOid(oid_t table_id, concurrency::Transaction *txn);

  // Read-only API
  std::string GetTableNameByOid(oid_t table_id, concurrency::Transaction *txn);
  std::string GetDatabaseNameByOid(oid_t table_id,
                                   concurrency::Transaction *txn);
  oid_t GetOidByName(const std::string &table_name,
                     const std::string &database_name,
                     concurrency::Transaction *txn);

  // TODO: Also add index for database oid?
  std::vector<oid_t> GetTableOidByDatabaseOid(oid_t database_id,
                                              concurrency::Transaction *txn);
  std::vector<std::string> GetTableNameByDatabaseOid(oid_t database_id,
                                              concurrency::Transaction *txn);

 private:
  TableCatalog(storage::Database *pg_catalog, type::AbstractPool *pool);

  ~TableCatalog();

  std::unique_ptr<catalog::Schema> InitializeSchema();
};

}  // End catalog namespace
}  // End peloton namespace
