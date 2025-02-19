#pragma once

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/write_batch.h>

#include <filesystem>
#include <functional>
#include <string_view>

#include "common/types.hpp"
#include "consensus/pbft_chain.hpp"
#include "dag/dag_block.hpp"
#include "dag/proposal_period_levels_map.hpp"
#include "logger/log.hpp"
#include "transaction_manager/transaction.hpp"
#include "transaction_manager/transaction_status.hpp"

namespace taraxa {
using namespace std;
using namespace dev;
using namespace rocksdb;
namespace fs = std::filesystem;

enum StatusDbField : uint8_t {
  ExecutedBlkCount = 0,
  ExecutedTrxCount,
  TrxCount,
  DagBlkCount,
  DagEdgeCount,
  DbMajorVersion,
  DbMinorVersion
};

enum PbftMgrRoundStep : uint8_t { PbftRound = 0, PbftStep };
enum PbftMgrStatus {
  soft_voted_block_in_round = 0,
  executed_block,
  executed_in_round,
  next_voted_soft_value,
  next_voted_null_block_hash,
};
enum PbftMgrVotedValue {
  own_starting_value_in_round = 0,
  soft_voted_block_hash_in_round,
};

enum DposProposalPeriodLevelsStatus : uint8_t { max_proposal_period = 0 };

class DbException : public exception {
 public:
  explicit DbException(string const& desc) : desc_(desc) {}
  virtual ~DbException() {}
  virtual const char* what() const noexcept { return desc_.c_str(); }

 private:
  string desc_;
};

struct DbStorage {
  using BatchPtr = shared_ptr<rocksdb::WriteBatch>;
  using OnEntry = function<bool(Slice const&, Slice const&)>;

  struct Column {
    string const name;
    size_t const ordinal;
  };

  class Columns {
   private:
    static inline vector<Column> all_;

   public:
    static inline auto const& all = all_;

    // Fix static initialization order fail by lazy initialization:
    // When rocksdb linked statically, rocksdb::kDefaultColumnFamilyName is empty during static initialization of
    // columns, therefore default_column, which depends on rocksdb::kDefaultColumnFamilyName must be lazy initialized
    static inline auto const Default_column() {
      static auto const default_column = all_.emplace_back(Column{kDefaultColumnFamilyName, all_.size()});
      return default_column;
    }

#define COLUMN(__name__) static inline auto const __name__ = all_.emplace_back(Column{#__name__, all_.size()})

    COLUMN(dag_blocks);
    COLUMN(dag_blocks_index);
    COLUMN(dag_blocks_state);
    // anchor_hash->[...dag_block_hashes_since_previous_anchor, anchor_hash]
    COLUMN(dag_finalized_blocks);
    COLUMN(transactions);
    // hash->dummy_short_value
    COLUMN(executed_transactions);
    COLUMN(trx_status);
    COLUMN(status);
    COLUMN(pbft_mgr_round_step);
    COLUMN(pbft_round_2t_plus_1);
    COLUMN(pbft_mgr_status);
    COLUMN(pbft_mgr_voted_value);
    COLUMN(pbft_cert_voted_block_hash);
    COLUMN(pbft_cert_voted_block);
    COLUMN(pbft_head);
    COLUMN(pbft_blocks);
    COLUMN(unverified_votes);
    COLUMN(verified_votes);
    COLUMN(soft_votes);  // only for current PBFT round
    COLUMN(cert_votes);  // for each PBFT block
    COLUMN(next_votes);  // only for previous PBFT round
    COLUMN(period_pbft_block);
    COLUMN(dag_block_period);
    COLUMN(dpos_proposal_period_levels_status);
    COLUMN(proposal_period_levels_map);
    COLUMN(replay_protection);
    COLUMN(pending_transactions);
    COLUMN(aleth_chain);
    COLUMN(aleth_chain_extras);

#undef COLUMN
  };

 private:
  fs::path path_;
  fs::path db_path_;
  fs::path state_db_path_;
  const std::string db_dir = "db";
  const std::string state_db_dir = "state_db";
  DB* db_;
  vector<ColumnFamilyHandle*> handles_;
  ReadOptions read_options_;
  WriteOptions write_options_;
  mutex dag_blocks_mutex_;
  atomic<uint64_t> dag_blocks_count_;
  atomic<uint64_t> dag_edge_count_;
  uint32_t db_snapshot_each_n_pbft_block_ = 0;
  uint32_t db_max_snapshots_ = 0;
  uint32_t snapshots_counter = 0;
  std::set<uint64_t> snapshots_;
  addr_t node_addr_;
  bool minor_version_changed_ = false;

  auto handle(Column const& col) const { return handles_[col.ordinal]; }

  LOG_OBJECTS_DEFINE

 public:
  DbStorage(DbStorage const&) = delete;
  DbStorage& operator=(DbStorage const&) = delete;

  explicit DbStorage(fs::path const& base_path, uint32_t db_snapshot_each_n_pbft_block = 0,
                     uint32_t db_max_snapshots = 0, uint32_t db_revert_to_period = 0, addr_t node_addr = addr_t(),
                     bool rebuild = 0);
  ~DbStorage();

  auto const& path() const { return path_; }
  auto dbStoragePath() const { return db_path_; }
  auto stateDbStoragePath() const { return state_db_path_; }
  static BatchPtr createWriteBatch();
  void commitWriteBatch(BatchPtr const& write_batch, rocksdb::WriteOptions const& opts);
  void commitWriteBatch(BatchPtr const& write_batch) { commitWriteBatch(write_batch, write_options_); }

  bool createSnapshot(uint64_t const& period);
  void deleteSnapshot(uint64_t const& period);
  void recoverToPeriod(uint64_t const& period);
  void loadSnapshots();

  // DAG
  void saveDagBlock(DagBlock const& blk, BatchPtr write_batch = nullptr);
  dev::bytes getDagBlockRaw(blk_hash_t const& hash);
  shared_ptr<DagBlock> getDagBlock(blk_hash_t const& hash);
  string getBlocksByLevel(level_t level);
  std::vector<std::shared_ptr<DagBlock>> getDagBlocksAtLevel(level_t level, int number_of_levels);

  // DAG state
  void addDagBlockStateToBatch(BatchPtr const& write_batch, blk_hash_t const& blk_hash, bool finalized);
  void removeDagBlockStateToBatch(BatchPtr const& write_batch, blk_hash_t const& blk_hash);
  std::map<blk_hash_t, bool> getAllDagBlockState();

  // Transaction
  void saveTransaction(Transaction const& trx);
  dev::bytes getTransactionRaw(trx_hash_t const& hash);
  shared_ptr<Transaction> getTransaction(trx_hash_t const& hash);
  shared_ptr<pair<Transaction, taraxa::bytes>> getTransactionExt(trx_hash_t const& hash);
  bool transactionInDb(trx_hash_t const& hash);
  void addTransactionToBatch(Transaction const& trx, BatchPtr const& write_batch);

  void saveTransactionStatus(trx_hash_t const& trx, TransactionStatus const& status);
  void addTransactionStatusToBatch(BatchPtr const& write_batch, trx_hash_t const& trx, TransactionStatus const& status);
  TransactionStatus getTransactionStatus(trx_hash_t const& hash);
  std::map<trx_hash_t, TransactionStatus> getAllTransactionStatus();

  // PBFT manager
  uint64_t getPbftMgrField(PbftMgrRoundStep const& field);
  void savePbftMgrField(PbftMgrRoundStep const& field, uint64_t const& value);
  void addPbftMgrFieldToBatch(PbftMgrRoundStep const& field, uint64_t const& value, BatchPtr const& write_batch);

  size_t getPbft2TPlus1(uint64_t const& pbft_round);
  void savePbft2TPlus1(uint64_t const& pbft_round, size_t const& pbft_2t_plus_1);
  void addPbft2TPlus1ToBatch(uint64_t const& pbft_round, size_t const& pbft_2t_plus_1, BatchPtr const& write_batch);

  bool getPbftMgrStatus(PbftMgrStatus const& field);
  void savePbftMgrStatus(PbftMgrStatus const& field, bool const& value);
  void addPbftMgrStatusToBatch(PbftMgrStatus const& field, bool const& value, BatchPtr const& write_batch);

  shared_ptr<blk_hash_t> getPbftMgrVotedValue(PbftMgrVotedValue const& field);
  void savePbftMgrVotedValue(PbftMgrVotedValue const& field, blk_hash_t const& value);
  void addPbftMgrVotedValueToBatch(PbftMgrVotedValue const& field, blk_hash_t const& value,
                                   BatchPtr const& write_batch);

  shared_ptr<blk_hash_t> getPbftCertVotedBlockHash(uint64_t const& pbft_round);
  void savePbftCertVotedBlockHash(uint64_t const& pbft_round, blk_hash_t const& cert_voted_block_hash);
  void addPbftCertVotedBlockHashToBatch(uint64_t const& pbft_round, blk_hash_t const& cert_voted_block_hash,
                                        BatchPtr const& write_batch);

  shared_ptr<PbftBlock> getPbftCertVotedBlock(blk_hash_t const& block_hash);
  void savePbftCertVotedBlock(PbftBlock const& pbft_block);
  void addPbftCertVotedBlockToBatch(PbftBlock const& pbft_block, BatchPtr const& write_batch);

  // pbft_blocks
  shared_ptr<PbftBlock> getPbftBlock(blk_hash_t const& hash);
  bool pbftBlockInDb(blk_hash_t const& hash);
  void addPbftBlockToBatch(PbftBlock const& pbft_block, BatchPtr const& write_batch);
  // pbft_blocks (head)
  // TODO: I would recommend storing this differently and not in the same db as
  // regular blocks with real hashes. Need remove from DB
  string getPbftHead(blk_hash_t const& hash);
  void savePbftHead(blk_hash_t const& hash, string const& pbft_chain_head_str);
  void addPbftHeadToBatch(taraxa::blk_hash_t const& head_hash, std::string const& head_str,
                          BatchPtr const& write_batch);
  // status
  uint64_t getStatusField(StatusDbField const& field);
  void saveStatusField(StatusDbField const& field, uint64_t const& value);
  void addStatusFieldToBatch(StatusDbField const& field, uint64_t const& value, BatchPtr const& write_batch);

  // Unverified votes
  std::vector<Vote> getUnverifiedVotes();
  shared_ptr<Vote> getUnverifiedVote(vote_hash_t const& vote_hash);
  bool unverifiedVoteExist(vote_hash_t const& vote_hash);
  void saveUnverifiedVote(Vote const& vote);
  void addUnverifiedVoteToBatch(Vote const& vote, BatchPtr const& write_batch);
  void removeUnverifiedVoteToBatch(vote_hash_t const& vote_hash, BatchPtr const& write_batch);

  // Verified votes
  std::vector<Vote> getVerifiedVotes();
  shared_ptr<Vote> getVerifiedVote(vote_hash_t const& vote_hash);
  void saveVerifiedVote(Vote const& vote);
  void addVerifiedVoteToBatch(Vote const& vote, BatchPtr const& write_batch);
  void removeVerifiedVoteToBatch(vote_hash_t const& vote_hash, BatchPtr const& write_batch);

  // Soft votes
  std::vector<Vote> getSoftVotes(uint64_t const& pbft_round);
  void saveSoftVotes(uint64_t const& pbft_round, std::vector<Vote> const& soft_votes);
  void addSoftVotesToBatch(uint64_t const& pbft_round, std::vector<Vote> const& soft_votes,
                           BatchPtr const& write_batch);
  void removeSoftVotesToBatch(uint64_t const& pbft_round, BatchPtr const& write_batch);

  // Certified votes
  std::vector<Vote> getCertVotes(blk_hash_t const& hash);
  void addCertVotesToBatch(taraxa::blk_hash_t const& pbft_block_hash, std::vector<Vote> const& cert_votes,
                           BatchPtr const& write_batch);
  // Next votes
  std::vector<Vote> getNextVotes(uint64_t const& pbft_round);
  void saveNextVotes(uint64_t const& pbft_round, std::vector<Vote> const& next_votes);
  void addNextVotesToBatch(uint64_t const& pbft_round, std::vector<Vote> const& next_votes,
                           BatchPtr const& write_batch);
  void removeNextVotesToBatch(uint64_t const& pbft_round, BatchPtr const& write_batch);

  // period_pbft_block
  shared_ptr<blk_hash_t> getPeriodPbftBlock(uint64_t const& period);
  void addPbftBlockPeriodToBatch(uint64_t const& period, taraxa::blk_hash_t const& pbft_block_hash,
                                 BatchPtr const& write_batch);
  // dag_block_period
  shared_ptr<uint64_t> getDagBlockPeriod(blk_hash_t const& hash);
  void addDagBlockPeriodToBatch(blk_hash_t const& hash, uint64_t const& period, BatchPtr const& write_batch);

  uint64_t getDagBlocksCount() const { return dag_blocks_count_.load(); }
  uint64_t getDagEdgeCount() const { return dag_edge_count_.load(); }

  auto getNumTransactionExecuted() { return getStatusField(StatusDbField::ExecutedTrxCount); }
  auto getNumTransactionInDag() { return getStatusField(StatusDbField::TrxCount); }
  auto getNumBlockExecuted() { return getStatusField(StatusDbField::ExecutedBlkCount); }
  uint64_t getNumDagBlocks() { return getDagBlocksCount(); }

  vector<blk_hash_t> getFinalizedDagBlockHashesByAnchor(blk_hash_t const& anchor);
  void putFinalizedDagBlockHashesByAnchor(WriteBatch& b, blk_hash_t const& anchor, vector<blk_hash_t> const& hs);

  // DPOS proposal period levels status
  uint64_t getDposProposalPeriodLevelsField(DposProposalPeriodLevelsStatus const& field);
  void saveDposProposalPeriodLevelsField(DposProposalPeriodLevelsStatus const& field, uint64_t const& value);
  void addDposProposalPeriodLevelsFieldToBatch(DposProposalPeriodLevelsStatus const& field, uint64_t const& value,
                                               BatchPtr const& write_batch);

  // DPOS proposal period to DAG block levels map
  bytes getProposalPeriodDagLevelsMap(uint64_t proposal_period);
  void saveProposalPeriodDagLevelsMap(ProposalPeriodDagLevelsMap const& period_levels_map);
  void addProposalPeriodDagLevelsMapToBatch(ProposalPeriodDagLevelsMap const& period_levels_map,
                                            BatchPtr const& write_batch);

  void insert(Column const& col, Slice const& k, Slice const& v);
  void remove(Slice key, Column const& column);
  void forEach(Column const& col, OnEntry const& f);

  bool hasMinorVersionChanged() { return minor_version_changed_; }

  inline static bytes asBytes(string const& b) {
    return bytes((byte const*)b.data(), (byte const*)(b.data() + b.size()));
  }

  inline static Slice toSlice(dev::bytesConstRef const& b) {
    return Slice(reinterpret_cast<char const*>(&b[0]), b.size());
  }

  template <unsigned N>
  inline static Slice toSlice(dev::FixedHash<N> const& h) {
    return {reinterpret_cast<char const*>(h.data()), N};
  }

  inline static Slice toSlice(dev::bytes const& b) { return toSlice(&b); }

  template <class N, typename = enable_if_t<is_arithmetic<N>::value>>
  inline static Slice toSlice(N const& n) {
    return Slice(reinterpret_cast<char const*>(&n), sizeof(N));
  }

  inline static Slice toSlice(string const& str) { return Slice(str.data(), str.size()); }

  inline static auto const& toSlice(Slice const& s) { return s; }

  template <typename T>
  inline static auto toSlices(std::vector<T> const& keys) {
    std::vector<Slice> ret;
    ret.reserve(keys.size());
    for (auto const& k : keys) {
      ret.emplace_back(toSlice(k));
    }
    return ret;
  }

  inline static auto const& toSlices(std::vector<Slice> const& ss) { return ss; }

  template <typename K>
  std::string lookup(K const& key, Column const& column) {
    std::string value;
    auto status = db_->Get(read_options_, handle(column), toSlice(key), &value);
    if (status.IsNotFound()) {
      return "";
    }
    checkStatus(status);
    return value;
  }

  template <typename K, typename V>
  void batch_put(WriteBatch& batch, Column const& col, K const& k, V const& v) {
    checkStatus(batch.Put(handle(col), toSlice(k), toSlice(v)));
  }

  // TODO remove
  void batch_put(BatchPtr const& batch, Column const& col, Slice const& k, Slice const& v) {
    batch_put(*batch, col, k, v);
  }

  // TODO generalize like batch_put
  void batch_delete(BatchPtr const& batch, Column const& col, Slice const& k) {
    checkStatus(batch->Delete(handle(col), k));
  }

  static void checkStatus(rocksdb::Status const& status);

  class MultiGetQuery {
    shared_ptr<DbStorage> const db_;
    vector<ColumnFamilyHandle*> cfs_;
    vector<Slice> keys_;
    vector<string> str_pool_;

   public:
    explicit MultiGetQuery(shared_ptr<DbStorage> const& db, uint capacity = 0);

    template <typename T>
    MultiGetQuery& append(Column const& col, vector<T> const& keys, bool copy_key = true) {
      auto h = db_->handle(col);
      for (auto const& k : keys) {
        cfs_.emplace_back(h);
        if (copy_key) {
          auto const& slice = toSlice(k);
          keys_.emplace_back(toSlice(str_pool_.emplace_back(slice.data(), slice.size())));
        } else {
          keys_.emplace_back(toSlice(k));
        }
      }
      return *this;
    }

    template <typename T>
    MultiGetQuery& append(Column const& col, T const& key, bool copy_key = true) {
      return append(col, vector{toSlice(key)}, copy_key);
    }

    dev::bytesConstRef get_key(uint pos);
    uint size();
    vector<string> execute(bool and_reset = true);
    MultiGetQuery& reset();
  };
};
}  // namespace taraxa
