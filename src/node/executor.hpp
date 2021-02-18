#pragma once

#include <atomic>

#include "chain/final_chain.hpp"
#include "consensus/pbft_chain.hpp"
#include "consensus/vote.hpp"
#include "dag/dag.hpp"
#include "network/rpc/WSServer.h"
#include "node/replay_protection_service.hpp"
#include "transaction_manager/transaction_manager.hpp"
#include "util/util.hpp"

namespace taraxa {

class Executor {
  std::mutex mu_;
  std::condition_variable cv_;
  std::shared_ptr<PbftBlock> to_execute_;

  std::unique_ptr<ReplayProtectionService> replay_protection_service_;
  std::shared_ptr<DbStorage> db_;
  std::shared_ptr<DagManager> dag_mgr_;
  std::shared_ptr<TransactionManager> trx_mgr_;
  std::shared_ptr<FinalChain> final_chain_;
  std::shared_ptr<PbftChain> pbft_chain_;
  std::shared_ptr<net::WSServer> ws_server_;

  addr_t node_addr_;
  std::atomic<bool> stopped_ = true;
  std::unique_ptr<std::thread> exec_worker_;

  // Containers used in executePbftBlocks_() as tmp data holders
  // optimization - they do not need to be always allocated from scratch
  std::vector<dev::eth::Transaction> trxs_tmp_buf_;
  std::vector<DagStats::TransactionStats> trxs_stats_tmp_buf_;
  DagStats dag_stats_tmp_buf_;

  std::atomic<uint64_t> num_executed_dag_blk_ = 0;
  std::atomic<uint64_t> num_executed_trx_ = 0;

  LOG_OBJECTS_DEFINE;

 public:
  Executor(addr_t node_addr, std::shared_ptr<DbStorage> db, std::shared_ptr<DagManager> dag_mgr,
           std::shared_ptr<TransactionManager> trx_mgr, std::shared_ptr<FinalChain> final_chain,
           std::shared_ptr<PbftChain> pbft_chain, uint32_t expected_max_trx_per_block);
  ~Executor();

  void setWSServer(std::shared_ptr<net::WSServer> ws_server);
  void start();
  void stop();

  void execute(std::shared_ptr<PbftBlock> blk);

 private:
  void tick();
  void execute_(PbftBlock const& blk);
  std::shared_ptr<PbftBlock> load_pbft_blk(uint64_t pbft_period);
  void clearTmpContainers();
};

}  // namespace taraxa
