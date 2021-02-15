#include "executor.hpp"

#include "config/config.hpp"
#include "dag/dag_block.hpp"
#include "dag_stats.hpp"

namespace taraxa {

struct ReplayProtectionServiceDummy : ReplayProtectionService {
  bool is_nonce_stale(addr_t const &addr, uint64_t nonce) const override { return false; }
  void update(DbStorage::BatchPtr batch, round_t round, util::RangeView<TransactionInfo> const &trxs) override {}
};

Executor::Executor(addr_t node_addr, std::shared_ptr<DbStorage> db, std::shared_ptr<DagManager> dag_mgr,
                   std::shared_ptr<TransactionManager> trx_mgr, std::shared_ptr<FinalChain> final_chain,
                   std::shared_ptr<PbftChain> pbft_chain, uint32_t expected_max_trx_per_block)
    : replay_protection_service_(new ReplayProtectionServiceDummy),
      node_addr_(node_addr),
      db_(db),
      dag_mgr_(dag_mgr),
      trx_mgr_(trx_mgr),
      final_chain_(final_chain),
      pbft_chain_(pbft_chain),
      txs_tmp_buf_(),
      txs_stats_tmp_buf_(),
      dag_stats_tmp_buf_(expected_max_trx_per_block) {
  LOG_OBJECTS_CREATE("EXECUTOR");
  if (auto last_period = pbft_chain->getPbftChainSize(); final_chain_->last_block_number() < last_period) {
    to_execute_ = load_pbft_blk(last_period);
  }
  num_executed_dag_blk_ = db_->getStatusField(taraxa::StatusDbField::ExecutedBlkCount);
  num_executed_trx_ = db_->getStatusField(taraxa::StatusDbField::ExecutedTrxCount);
  txs_tmp_buf_.reserve(expected_max_trx_per_block);
  txs_stats_tmp_buf_.reserve(expected_max_trx_per_block);
}

Executor::~Executor() { stop(); }

void Executor::setWSServer(std::shared_ptr<net::WSServer> ws_server) { ws_server_ = move(ws_server); }

void Executor::start() {
  if (bool b = true; !stopped_.compare_exchange_strong(b, !b)) {
    return;
  }
  LOG(log_nf_) << "Executor start...";
  exec_worker_ = std::make_unique<std::thread>([this]() {
    LOG(log_nf_) << "Executor run...";
    while (!stopped_) {
      tick();
    }
  });
}

void Executor::stop() {
  if (bool b = false; !stopped_.compare_exchange_strong(b, !b)) {
    return;
  }
  cv_.notify_one();
  exec_worker_->join();
  LOG(log_nf_) << "Executor stopped";
}

void Executor::execute(std::shared_ptr<PbftBlock> blk) {
  std::unique_lock l(mu_);
  to_execute_ = move(blk);
  cv_.notify_one();
}

void Executor::tick() {
  std::shared_ptr<PbftBlock> pbft_block;
  {
    std::unique_lock l(mu_);
    if (!to_execute_) {
      cv_.wait(l);
    }
    if (stopped_) {
      return;
    }
    std::swap(pbft_block, to_execute_);
  }
  execute_(*pbft_block);
  for (auto blk_n = final_chain_->last_block_number(); blk_n < pbft_block->getPeriod(); ++blk_n) {
    execute_(*load_pbft_blk(blk_n));
  }
}

void Executor::execute_(PbftBlock const &pbft_block) {
  auto pbft_period = pbft_block.getPeriod();
  auto const &pbft_block_hash = pbft_block.getBlockHash();
  auto const &anchor_hash = pbft_block.getPivotDagBlockHash();
  auto finalized_dag_blk_hashes = db_->getFinalizedDagBlockHashesByAnchor(anchor_hash);
  auto batch = db_->createWriteBatch();
  {
    // This artificial scope will make sure we clean up the big chunk of memory allocated for this batch-processing
    // stuff as soon as possible
    DbStorage::MultiGetQuery db_query(db_, txs_tmp_buf_.capacity() + 100);
    auto dag_blks_raw = db_query.append(DbStorage::Columns::dag_blocks, finalized_dag_blk_hashes, false).execute();

    for (auto const &dag_blk_raw : dag_blks_raw) {
      const auto &dag_blk_rlp = RLP(dag_blk_raw);
      DagBlock dag_block(dag_blk_rlp);  // Full block has to be decoded from rlp to get block author from signature
      // TODO: Check if dag block is validated to consist at least 1 tx. Is this checked before ???

      addr_t dag_block_author = dag_block.getSender();

      for (auto const &tx_hash : dag_block.getTrxs()) {
        if (dag_stats_tmp_buf_.addTransaction(tx_hash, dag_block_author)) {
          // Tx was already processed
          continue;
        }

        db_query.append(DbStorage::Columns::executed_transactions, tx_hash);
        db_query.append(DbStorage::Columns::transactions, tx_hash);
      }

      // TODO: there is a possibility that some miner includes only txs that are later in filtered out due to
      // replay_protection_service_ or "has_been_executed", but gets rewards for dag block creation
      // It might or might not be ok, he provided valid block and validated therefore other blocks so he did
      // some work, on the other hand none of the txs that he included were actually processed...
      dag_stats_tmp_buf_.addDagBlock(dag_block_author);
    }

    auto trx_db_results = db_query.execute(false);
    for (uint i = 0; i < trx_db_results.size() / 2; ++i) {
      size_t executed_tx_result_idx = 0 + i * 2;
      size_t tx_result_idx = 1 + i * 2;

      h256 tx_hash = h256(db_query.get_key(tx_result_idx));

      auto has_not_been_saved = trx_db_results[tx_result_idx].empty();
      if (has_not_been_saved) {
        // This should never happen
        throw std::runtime_error("Executor::executePbftBlocks_: invalid transaction is being processed: tx_hash: " +
                                 tx_hash.toString());
      }

      bool has_been_executed = !trx_db_results[executed_tx_result_idx].empty();
      if (has_been_executed) {
        continue;
      }

      // Non-executed txs
      auto tx = dev::eth::Transaction(&trx_db_results[tx_result_idx], dev::eth::CheckTransaction::None, true, tx_hash);

      if (replay_protection_service_->is_nonce_stale(tx.sender(), tx.nonce())) {
        continue;
      }

      txs_tmp_buf_.emplace_back(std::move(tx));
      txs_stats_tmp_buf_.emplace_back(dag_stats_tmp_buf_.getTransactionStatsRvalue(tx_hash));

      static string const dummy_val = "_";
      db_->batch_put(*batch, DbStorage::Columns::executed_transactions, tx.sha3(), dummy_val);
    }
  }

  // Execute transactions in EVM(GO trx engine) and update Ethereum block
  auto const &[new_eth_header, trx_receipts, _] =
      final_chain_->advance(batch, pbft_block.getBeneficiary(), pbft_block.getTimestamp(), txs_tmp_buf_,
                            txs_stats_tmp_buf_, dag_stats_tmp_buf_.getBlocksStats());

  // Update replay protection service, like nonce watermark. Nonce watermark has been disabled
  replay_protection_service_->update(batch, pbft_period, util::make_range_view(txs_tmp_buf_).map([](auto const &trx) {
    return ReplayProtectionService::TransactionInfo{
        trx.from(),
        trx.nonce(),
    };
  }));

  // Update number of executed DAG blocks and transactions
  auto num_executed_dag_blk = num_executed_dag_blk_ + finalized_dag_blk_hashes.size();
  auto num_executed_trx = num_executed_trx_ + txs_tmp_buf_.size();
  if (!finalized_dag_blk_hashes.empty()) {
    db_->addStatusFieldToBatch(StatusDbField::ExecutedBlkCount, num_executed_dag_blk, batch);
    db_->addStatusFieldToBatch(StatusDbField::ExecutedTrxCount, num_executed_trx, batch);
    LOG(log_nf_) << node_addr_ << " :   Executed dag blocks index #"
                 << num_executed_dag_blk_ - finalized_dag_blk_hashes.size() << "-" << num_executed_dag_blk_ - 1
                 << " , Transactions count: " << txs_tmp_buf_.size();
  }

  // Remove executed transactions at Ethereum pending block. The Ethereum pending block is same with latest block at
  // Taraxa
  trx_mgr_->getPendingBlock()->advance(
      batch, new_eth_header.hash(),
      util::make_range_view(txs_tmp_buf_).map([](auto const &trx) { return trx.sha3(); }));

  // Commit DB
  db_->commitWriteBatch(batch);
  LOG(log_nf_) << "DB write batch committed at period " << pbft_period << " PBFT block hash " << pbft_block_hash;

  // After DB commit, confirm in final chain(Ethereum)
  final_chain_->advance_confirm();

  // Only NOW we are fine to modify in-memory states as they have been backed by the db

  num_executed_dag_blk_ = num_executed_dag_blk;
  num_executed_trx_ = num_executed_trx;

  // Creates snapshot if needed
  if (db_->createSnapshot(pbft_period)) {
    final_chain_->create_snapshot(pbft_period);
  }

  // Ethereum filter
  trx_mgr_->getFilterAPI()->note_block(new_eth_header.hash());
  trx_mgr_->getFilterAPI()->note_receipts(trx_receipts);

  // Update web server
  if (ws_server_) {
    ws_server_->newDagBlockFinalized(anchor_hash, pbft_period);
    ws_server_->newPbftBlockExecuted(pbft_block, finalized_dag_blk_hashes);
    ws_server_->newEthBlock(new_eth_header);
  }

  clearTmpContainers();

  LOG(log_nf_) << node_addr_ << " successful execute pbft block " << pbft_block_hash << " in period " << pbft_period;
}

std::shared_ptr<PbftBlock> Executor::load_pbft_blk(uint64_t pbft_period) {
  auto pbft_block_hash = db_->getPeriodPbftBlock(pbft_period);
  if (!pbft_block_hash) {
    LOG(log_er_) << "DB corrupted - PBFT block period " << pbft_period
                 << " does not exist in DB period_pbft_block. PBFT chain size " << pbft_chain_->getPbftChainSize();
    assert(false);
  }
  // Get PBFT block in DB
  auto pbft_block = db_->getPbftBlock(*pbft_block_hash);
  if (!pbft_block) {
    LOG(log_er_) << "DB corrupted - Cannot find PBFT block hash " << pbft_block_hash
                 << " in PBFT chain DB pbft_blocks.";
    assert(false);
  }
  if (pbft_block->getPeriod() != pbft_period) {
    LOG(log_er_) << "DB corrupted - PBFT block hash " << pbft_block_hash << "has different period "
                 << pbft_block->getPeriod() << " in block data than in block order db: " << pbft_period;
    assert(false);
  }
  return pbft_block;
}

void Executor::clearTmpContainers() {
  txs_tmp_buf_.clear();
  txs_stats_tmp_buf_.clear();
  dag_stats_tmp_buf_.clear();
}

}  // namespace taraxa
