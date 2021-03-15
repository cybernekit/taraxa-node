#include "taraxa_capability.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

#include "consensus/pbft_chain.hpp"
#include "consensus/pbft_manager.hpp"
#include "consensus/vote.hpp"
#include "dag/dag.hpp"
#include "node/full_node.hpp"
#include "transaction_manager/transaction_manager.hpp"

using namespace taraxa;

void TaraxaCapability::sealAndSend(NodeID const &nodeID, RLPStream &s, unsigned packet_type) {
  try {
    if (conf_.network_performance_log) {
      const auto time = std::chrono::system_clock::now();
      std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
      auto packet_size = s.out().size();

      host_.capabilityHost()->sealAndSend(nodeID, s);

      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin);
      PacketStats packet_stats{nodeID, time, packet_size, duration};

      if (conf_.network_performance_log_interval) {
        perf_sent_packets_stats_.addPacket(packet_type, packet_stats);
      }

      LOG(log_dg_net_per_) << "(\"" << host_.id() << "\") sent " << packetToPacketName(packet_type) << " packet to (\""
                           << nodeID << "\"). Stats: " << packet_stats;
    } else {
      host_.capabilityHost()->sealAndSend(nodeID, s);
    }
  } catch (const std::exception &e) {
    LOG(log_er_) << "Caught exception in sealAndSend: " << e.what();
    throw;
  } catch (...) {
    LOG(log_er_) << "Caught unknown exception in sealAndSend.";
    throw;
  }
}

std::shared_ptr<TaraxaPeer> TaraxaCapability::getPeer(NodeID const &node_id) {
  boost::shared_lock<boost::shared_mutex> lock(peers_mutex_);
  auto itPeer = peers_.find(node_id);
  if (itPeer != peers_.end()) {
    return itPeer->second;
  }
  return nullptr;
}

unsigned int TaraxaCapability::getPeersCount() {
  boost::shared_lock<boost::shared_mutex> lock(peers_mutex_);
  return peers_.size();
}

void TaraxaCapability::erasePeer(NodeID const &node_id) {
  boost::unique_lock<boost::shared_mutex> lock(peers_mutex_);
  peers_.erase(node_id);
}

void TaraxaCapability::insertPeer(NodeID const &node_id, std::shared_ptr<TaraxaPeer> const &peer) {
  boost::unique_lock<boost::shared_mutex> lock(peers_mutex_);
  peers_.emplace(std::make_pair(node_id, std::make_shared<TaraxaPeer>(node_id)));
}

void TaraxaCapability::syncPeerPbft(NodeID const &_nodeID, unsigned long height_to_sync) {
  LOG(log_nf_pbft_sync_) << "Sync peer node " << _nodeID << " from pbft chain height " << height_to_sync;
  requestPbftBlocks(_nodeID, height_to_sync);
}

std::pair<bool, blk_hash_t> TaraxaCapability::checkDagBlockValidation(DagBlock const &block) {
  if (dag_blk_mgr_->getDagBlock(block.getHash())) {
    // The DAG block exist
    return std::make_pair(true, blk_hash_t());
  }

  level_t expected_level = 0;
  for (auto const &tip : block.getTips()) {
    auto tip_block = dag_blk_mgr_->getDagBlock(tip);
    if (!tip_block) {
      LOG(log_nf_dag_sync_) << "Block " << block.getHash().toString() << " has a missing tip " << tip.toString();
      return std::make_pair(false, tip);
    }
    expected_level = std::max(tip_block->getLevel(), expected_level);
  }
  auto pivot = block.getPivot();
  auto pivot_block = dag_blk_mgr_->getDagBlock(pivot);
  if (!pivot_block) {
    LOG(log_nf_) << "Block " << block.getHash().toString() << " has a missing pivot " << pivot.toString();
    return std::make_pair(false, pivot);
  }
  expected_level = std::max(pivot_block->getLevel(), expected_level);
  expected_level++;
  if (expected_level != block.getLevel()) {
    throw InvalidDataException(std::string("Invalid block level ") + std::to_string(block.getLevel()) + " for block " +
                               block.getHash().toString() + ". Expected level " + std::to_string(expected_level));
  }

  return std::make_pair(true, blk_hash_t());
}

void TaraxaCapability::onConnect(NodeID const &_nodeID, u256 const &) {
  LOG(log_nf_) << "Node " << _nodeID << " connected";
  cnt_received_messages_[_nodeID] = 0;
  test_sums_[_nodeID] = 0;

  insertPeer(_nodeID, std::make_shared<TaraxaPeer>(_nodeID));
  sendStatus(_nodeID, true);
}

bool TaraxaCapability::interpretCapabilityPacket(NodeID const &_nodeID, unsigned _id, RLP const &_r) {
  try {
    if (stopped_) return true;

    if (conf_.network_simulated_delay != 0) {
      // RLP contains memory it does not own so deep copy of bytes is needed
      dev::bytes rBytes = _r.data().toBytes();
      int messageSize = rBytes.size() * 8;
      unsigned int dist = *((int *)this->host_.id().data()) ^ *((int *)_nodeID.data());
      unsigned int delay = dist % conf_.network_simulated_delay;

      auto bandwidth = conf_.network_bandwidth ? conf_.network_bandwidth : 40;
      unsigned int bandwidth_delay = messageSize / (bandwidth * 1000);  // in ms

      // Random component up to +-10%
      int random_component = random_dist_(delay_rng_);

      unsigned int total_delay = (delay + bandwidth_delay) * random_component / 100;
      LOG(log_dg_) << "Delaying packet by: (" << delay << " , " << bandwidth_delay
                   << " ), actual delay =" << total_delay << " milliseconds";
      auto timer = std::make_shared<boost::asio::deadline_timer>(io_service_);
      timer->expires_from_now(boost::posix_time::milliseconds(total_delay));
      timer->async_wait(([this, _nodeID, _id, rBytes, timer](const boost::system::error_code &ec) {
        RLP _rCopy(rBytes);
        interpretCapabilityPacketImpl(_nodeID, _id, _rCopy);
      }));

      return true;
    }

    if (conf_.network_performance_log) {
      const auto time = std::chrono::system_clock::now();
      std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

      auto ret = interpretCapabilityPacketImpl(_nodeID, _id, _r);

      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin);
      PacketStats packet_stats{_nodeID, time, _r.actualSize(), duration};

      if (conf_.network_performance_log_interval) {
        perf_received_packets_stats_.addPacket(_id, packet_stats);
      }

      LOG(log_dg_net_per_) << "(\"" << host_.id() << "\") received " << packetToPacketName(_id) << " packet from (\""
                           << _nodeID << "\"). Stats: " << packet_stats;

      return ret;
    }

    return interpretCapabilityPacketImpl(_nodeID, _id, _r);
  } catch (const std::exception &e) {
    LOG(log_er_) << "Caught exception in interpretCapabilityPacket: " << e.what();
    throw;
  } catch (...) {
    LOG(log_er_) << "Caught unknown exception in sealAndSend.";
    throw;
  }
}

#define __DBG__(_rlp)                                                                                 \
  [&](auto const &val) {                                                                              \
    try {                                                                                             \
      return val.toPositiveInt64();                                                                   \
    } catch (std::exception const &_e) {                                                              \
      LOG(log_er_) << "RANDOM_TAG line: " << __LINE__ << ", rlp value: " << dev::toHex(val.toBytes()) \
                   << ", err: " << _e.what();                                                         \
      throw;                                                                                          \
    }                                                                                                 \
  }(_rlp)

bool TaraxaCapability::interpretCapabilityPacketImpl(NodeID const &_nodeID, unsigned _id, RLP const &_r) {
  try {
    auto peer = getPeer(_nodeID);
    if (peer) {
      packet_size[_id] += _r.actualSize();
      packet_count[_id]++;

      switch (_id) {
        case SyncedPacket: {
          LOG(log_dg_dag_sync_) << "Received synced message from " << _nodeID;

          peer->syncing_ = false;
          peer->clearAllKnownBlocksAndTransactions();

          break;
        }
        case StatusPacket: {
          if (_r.itemCount() != 5 && _r.itemCount() != 10) {
            LOG(log_er_) << "Malformed StatusPacket. bytes: " << _r.data() << ", itemCount: " << _r.itemCount();
          }

          peer->statusReceived();
          bool initial_status = _r.itemCount() == 10;
          auto pbft_chain_size = pbft_chain_->getPbftChainSize();

          LOG(log_dg_) << "RANDOM_TAG [Node " << host_.id() << "] received StatusPacket 1/2 from [Node " << _nodeID
                       << "]: "
                       << "bytes: " << _r.data() << ", itemCount: " << _r.itemCount();

          if (initial_status) {
            auto it = _r.begin();

            auto const peer_protocol_version = (*it++).toInt<unsigned>();
            auto const network_id = __DBG__(*it++);
            peer->dag_level_ = __DBG__(*it++);
            auto const genesis_hash = (*it++).toString();
            peer->pbft_chain_size_ = __DBG__(*it++);
            peer->syncing_ = (*it++).toInt();
            peer->pbft_round_ = __DBG__(*it++);
            peer->pbft_previous_round_next_votes_size_ = (*it++).toInt<unsigned>();
            auto node_major_version = (*it++).toInt();
            auto node_minor_version = (*it++).toInt();

            LOG(log_dg_) << "RANDOM_TAG [Node " << host_.id() << "] received initial StatusPacket 2/2 from [Node "
                         << _nodeID << "]: "
                         << "data: " << peer_protocol_version << ", " << network_id << ", " << peer->dag_level_ << ", "
                         << genesis_hash << ", " << peer->pbft_chain_size_ << ", " << peer->syncing_ << ", "
                         << peer->pbft_round_ << ", " << peer->pbft_previous_round_next_votes_size_ << ", "
                         << node_major_version << ", " << node_minor_version;

            //            LOG(log_dg_) << "Received initial status message from " << _nodeID << ", peer protocol version
            //            "
            //                         << peer_protocol_version << ", network id " << network_id << ", peer DAG max
            //                         level "
            //                         << peer->dag_level_ << ", genesis " << genesis_ << ", peer pbft chain size "
            //                         << peer->pbft_chain_size_ << ", peer syncing " << peer->syncing_ << ", peer pbft
            //                         round "
            //                         << peer->pbft_round_ << ", peer pbft previous round next votes size "
            //                         << peer->pbft_previous_round_next_votes_size_ << ", node major version" <<
            //                         node_major_version
            //                         << ", node minor version" << node_minor_version;

            if (peer_protocol_version != FullNode::c_network_protocol_version) {
              LOG(log_er_) << "Incorrect protocol version " << peer_protocol_version << ", host " << _nodeID
                           << " will be disconnected";
              host_.capabilityHost()->disconnect(_nodeID, p2p::UserReason);
            }
            // We need logic when some different node versions might still be compatible
            if (node_major_version != FullNode::c_node_major_version ||
                node_minor_version != FullNode::c_node_minor_version) {
              LOG(log_er_) << "Incorrect node version: " << getFormattedVersion(node_major_version, node_minor_version)
                           << ", our node major version"
                           << getFormattedVersion(FullNode::c_node_major_version, FullNode::c_node_minor_version)
                           << ", host " << _nodeID << " will be disconnected";
              host_.capabilityHost()->disconnect(_nodeID, p2p::UserReason);
            }
            if (network_id != conf_.network_id) {
              LOG(log_er_) << "Incorrect network id " << network_id << ", host " << _nodeID << " will be disconnected";
              host_.capabilityHost()->disconnect(_nodeID, p2p::UserReason);
            }
            if (genesis_hash != genesis_) {
              LOG(log_er_) << "Incorrect genesis hash " << genesis_hash << ", host " << _nodeID
                           << " will be disconnected";
              host_.capabilityHost()->disconnect(_nodeID, p2p::UserReason);
            }
            // Only on the initial status message the other node might not
            // have still started syncing so double check with pbft chain size
            peer->syncing_ |= peer->pbft_chain_size_ < pbft_chain_size;
          } else {
            auto it = _r.begin();
            peer->dag_level_ = __DBG__(*it++);
            peer->pbft_chain_size_ = __DBG__(*it++);
            peer->syncing_ = (*it++).toInt();
            peer->pbft_round_ = __DBG__(*it++);
            peer->pbft_previous_round_next_votes_size_ = (*it++).toInt<unsigned>();

            LOG(log_dg_) << "RANDOM_TAG [Node << " << host_.id() << "] received standard StatusPacket 2/2 from [Node "
                         << _nodeID << "]: "
                         << ", data: " << peer->dag_level_ << ", " << peer->pbft_chain_size_ << ", " << peer->syncing_
                         << ", " << peer->pbft_round_ << ", " << peer->pbft_previous_round_next_votes_size_;

            //            LOG(log_dg_) << "Received status message from " << _nodeID << ", peer DAG max level " <<
            //            peer->dag_level_
            //                         << ", peer pbft chain size " << peer->pbft_chain_size_ << ", peer syncing " <<
            //                         peer->syncing_
            //                         << ", peer pbft round " << peer->pbft_round_ << ", peer pbft previous round next
            //                         votes size "
            //                         << peer->pbft_previous_round_next_votes_size_;
          }

          LOG(log_dg_dag_sync_) << "Received status message from " << _nodeID
                                << " peer DAG max level:" << peer->dag_level_;
          LOG(log_dg_pbft_sync_) << "Received status message from " << _nodeID << ", peer sycning: " << peer->syncing_
                                 << ", PBFT chain size:" << peer->pbft_chain_size_;
          LOG(log_dg_next_votes_sync_) << "Received status message from " << _nodeID << ", PBFT round "
                                       << peer->pbft_round_ << ", peer PBFT previous round next votes size "
                                       << peer->pbft_previous_round_next_votes_size_;

          if (peer->syncing_) {
            if (syncing_ && peer_syncing_pbft_ == _nodeID) {
              // We are currently syncing to a node that just reported it is not synced, force a switch to a new node
              LOG(log_nf_) << "Restart syncing, currently syncing from the node " << _nodeID
                           << " that is itself syncing";
              restartSyncingPbft(true);
            }
          }

          auto pbft_current_round = pbft_mgr_->getPbftRound();
          auto pbft_previous_round_next_votes_size = next_votes_mgr_->getNextVotesSize();
          if (pbft_current_round < peer->pbft_round_ ||
              (pbft_current_round == peer->pbft_round_ &&
               pbft_previous_round_next_votes_size < peer->pbft_previous_round_next_votes_size_)) {
            syncPbftNextVotes(pbft_current_round, pbft_previous_round_next_votes_size);
          }

          break;
        }
        // Means a new block is proposed, full block body and all transaction
        // are received.
        case NewBlockPacket: {
          DagBlock block(_r[0].data().toBytes());

          if (dag_blk_mgr_) {
            if (dag_blk_mgr_->isBlockKnown(block.getHash())) {
              LOG(log_dg_dag_prp_) << "Received NewBlock " << block.getHash().toString() << "that is already known";
              break;
            }
          }
          unique_packet_count[_id]++;

          auto transactionsCount = _r.itemCount() - 1;
          LOG(log_dg_dag_prp_) << "Received NewBlockPacket " << transactionsCount;

          std::vector<Transaction> newTransactions;
          for (auto iTransaction = 1; iTransaction < transactionsCount + 1; iTransaction++) {
            Transaction transaction(_r[iTransaction].data().toBytes());
            newTransactions.push_back(transaction);
            peer->markTransactionAsKnown(transaction.getHash());
          }

          peer->markBlockAsKnown(block.getHash());
          if (block.getLevel() > peer->dag_level_) peer->dag_level_ = block.getLevel();
          onNewBlockReceived(block, newTransactions);
          break;
        }

        case NewBlockHashPacket: {
          blk_hash_t hash(_r[0]);
          LOG(log_dg_dag_prp_) << "Received NewBlockHashPacket" << hash.toString();
          peer->markBlockAsKnown(hash);
          if (dag_blk_mgr_) {
            if (!dag_blk_mgr_->isBlockKnown(hash) && block_requestes_set_.count(hash) == 0) {
              unique_packet_count[_id]++;
              block_requestes_set_.insert(hash);
              requestBlock(_nodeID, hash);
            }
          } else if (test_blocks_.find(hash) == test_blocks_.end() && block_requestes_set_.count(hash) == 0) {
            block_requestes_set_.insert(hash);
            requestBlock(_nodeID, hash);
          }
          break;
        }
        case GetNewBlockPacket: {
          blk_hash_t hash(_r[0]);
          peer->markBlockAsKnown(hash);
          LOG(log_dg_dag_prp_) << "Received GetNewBlockPacket" << hash.toString();

          if (dag_blk_mgr_) {
            auto block = db_->getDagBlock(hash);
            if (block) {
              sendBlock(_nodeID, *block);
            } else
              LOG(log_nf_dag_prp_) << "NO NEW PACKET: " << hash.toString();
          } else if (test_blocks_.find(hash) != test_blocks_.end()) {
            sendBlock(_nodeID, test_blocks_[hash]);
          }
          break;
        }
        case GetBlocksPacket: {
          LOG(log_dg_dag_sync_) << "Received GetBlocksPacket";
          std::vector<std::shared_ptr<DagBlock>> dag_blocks;
          auto blocks = dag_mgr_->getNonFinalizedBlocks();
          for (auto &level_blocks : blocks) {
            for (auto &block : level_blocks.second) {
              dag_blocks.emplace_back(db_->getDagBlock(blk_hash_t(block)));
            }
          }
          sendBlocks(_nodeID, dag_blocks);
          break;
        }
        case BlocksPacket: {
          std::string received_dag_blocks_str;
          auto itemCount = _r.itemCount();
          int transactionCount = 0;
          requesting_pending_dag_blocks_ = false;
          for (auto iBlock = 0; iBlock < itemCount; iBlock++) {
            DagBlock block(_r[iBlock + transactionCount].data().toBytes());
            peer->markBlockAsKnown(block.getHash());

            std::vector<Transaction> newTransactions;
            for (int i = 0; i < block.getTrxs().size(); i++) {
              transactionCount++;
              Transaction transaction(_r[iBlock + transactionCount].data().toBytes());
              newTransactions.push_back(transaction);
              peer->markTransactionAsKnown(transaction.getHash());
            }

            received_dag_blocks_str += block.getHash().toString() + " ";

            auto status = checkDagBlockValidation(block);
            if (!status.first) {
              LOG(log_nf_dag_sync_) << "DagBlockValidation failed " << status.second;
              break;
            }

            LOG(log_nf_dag_sync_) << "Storing block " << block.getHash().toString() << " with "
                                  << newTransactions.size() << " transactions";
            if (block.getLevel() > peer->dag_level_) peer->dag_level_ = block.getLevel();
            dag_blk_mgr_->insertBroadcastedBlockWithTransactions(block, newTransactions);

            if (iBlock + transactionCount + 1 >= itemCount) break;
          }

          LOG(log_nf_dag_sync_) << "Received Dag Blocks: " << received_dag_blocks_str;
          break;
        }
        case TransactionPacket: {
          std::string receivedTransactions;
          std::vector<taraxa::bytes> transactions;
          auto transactionCount = _r.itemCount();
          for (auto iTransaction = 0; iTransaction < transactionCount; iTransaction++) {
            Transaction transaction(_r[iTransaction].data().toBytes());
            receivedTransactions += transaction.getHash().toString() + " ";
            peer->markTransactionAsKnown(transaction.getHash());
            transactions.emplace_back(_r[iTransaction].data().toBytes());
          }
          if (transactionCount > 0) {
            LOG(log_dg_trx_prp_) << "Received TransactionPacket with " << _r.itemCount() << " transactions";
            LOG(log_tr_trx_prp_) << "Received TransactionPacket with " << _r.itemCount()
                                 << " transactions:" << receivedTransactions.c_str();

            onNewTransactions(transactions, true);
          }
          break;
        }
        case PbftVotePacket: {
          LOG(log_dg_vote_prp_) << "In PbftVotePacket";

          Vote vote(_r[0].toBytes());
          LOG(log_dg_vote_prp_) << "Received PBFT vote " << vote.getHash();
          peer->markVoteAsKnown(vote.getHash());

          if (vote_mgr_->addVote(vote)) {
            unique_packet_count[_id]++;
            onNewPbftVote(vote);
          }
          break;
        }
        case GetPbftNextVotes: {
          LOG(log_dg_next_votes_sync_) << "Received GetPbftNextVotes request";

          uint64_t peer_pbft_round = __DBG__(_r[0]);
          size_t peer_pbft_previous_round_next_votes_size = _r[1].toInt<unsigned>();
          uint64_t pbft_round = pbft_mgr_->getPbftRound();
          size_t pbft_previous_round_next_votes_size = next_votes_mgr_->getNextVotesSize();

          if (pbft_round > peer_pbft_round ||
              (pbft_round == peer_pbft_round &&
               pbft_previous_round_next_votes_size > peer_pbft_previous_round_next_votes_size)) {
            LOG(log_dg_next_votes_sync_) << "Current PBFT round is " << pbft_round << " previous round next votes size "
                                         << pbft_previous_round_next_votes_size << ", and peer PBFT round is "
                                         << peer_pbft_round << " previous round next votes size "
                                         << peer_pbft_previous_round_next_votes_size
                                         << ". Will send out bundle of next votes";
            sendPbftNextVotes(_nodeID);
          }

          break;
        }
        case PbftNextVotesPacket: {
          auto next_votes_count = _r.itemCount();
          if (next_votes_count == 0) {
            LOG(log_er_next_votes_sync_) << "Receive 0 next votes from peer " << _nodeID
                                         << ". The peer may be a malicous player, will be disconnected";
            host_.capabilityHost()->disconnect(_nodeID, p2p::UserReason);

            break;
          }
          LOG(log_nf_next_votes_sync_) << "Received " << next_votes_count << " next votes from peer " << _nodeID;

          std::vector<Vote> next_votes;
          for (auto i = 0; i < next_votes_count; i++) {
            Vote next_vote(_r[i].data().toBytes());
            LOG(log_nf_next_votes_sync_) << "Received PBFT next vote " << next_vote.getHash();

            next_votes.emplace_back(next_vote);
          }

          auto pbft_current_round = pbft_mgr_->getPbftRound();
          auto pbft_previous_round_next_votes_size = next_votes_mgr_->getNextVotesSize();
          auto peer_pbft_round = next_votes[0].getRound() + 1;

          if (pbft_current_round < peer_pbft_round) {
            // Add into votes unverified queue
            vote_mgr_->addVotes(next_votes);
          } else if (pbft_current_round == peer_pbft_round) {
            // Update previous round next votes
            auto pbft_2t_plus_1 = db_->getPbft2TPlus1(pbft_current_round - 1);
            if (pbft_2t_plus_1) {
              next_votes_mgr_->updateWithSyncedVotes(next_votes, pbft_2t_plus_1);
            } else {
              LOG(log_er_) << "Cannot get PBFT 2t+1 in PBFT round " << pbft_current_round - 1;
            }
          }

          break;
        }
        case GetPbftBlockPacket: {
          size_t height_to_sync = _r[0].toInt();
          size_t my_chain_size = pbft_chain_->getPbftChainSize();
          size_t blocks_to_transfer = 0;
          if (my_chain_size >= height_to_sync) {
            blocks_to_transfer =
                std::min((uint64_t)conf_.network_sync_level_size, (uint64_t)(my_chain_size - (height_to_sync - 1)));
          }
          LOG(log_dg_pbft_sync_) << "Send pbftblocks to " << _nodeID;
          sendPbftBlocks(_nodeID, height_to_sync, blocks_to_transfer);
          break;
        }
        // no cert vote needed (propose block)
        case NewPbftBlockPacket: {
          LOG(log_dg_pbft_prp_) << "In NewPbftBlockPacket";

          auto pbft_block = s_ptr(new PbftBlock(_r[0]));
          uint64_t pbft_chain_size = _r[1].toInt();
          LOG(log_dg_pbft_prp_) << "Receive proposed PBFT Block " << pbft_block
                                << " Peer Chain size: " << pbft_chain_size;
          peer->markPbftBlockAsKnown(pbft_block->getBlockHash());
          if (pbft_chain_size > peer->pbft_chain_size_) peer->pbft_chain_size_ = pbft_chain_size;

          if (pbft_chain_ && !pbft_chain_->findUnverifiedPbftBlock(pbft_block->getBlockHash())) {
            // TODO: need to check block validation, like proposed
            // vote(maybe
            //  come later), if get sortition etc
            unique_packet_count[_id]++;
            pbft_chain_->pushUnverifiedPbftBlock(pbft_block);
            onNewPbftBlock(*pbft_block);
          }
          break;
        }
        // need cert votes (syncing)
        case PbftBlockPacket: {
          auto pbft_blk_count = _r.itemCount();
          LOG(log_dg_pbft_sync_) << "In PbftBlockPacket received, num pbft blocks: " << pbft_blk_count;
          pbft_sync_period_ = pbft_chain_->pbftSyncingPeriod();
          for (auto const &pbft_blk_tuple : _r) {
            PbftBlockCert pbft_blk_and_votes(pbft_blk_tuple[0]);
            LOG(log_dg_pbft_sync_) << "Received pbft block: " << pbft_blk_and_votes.pbft_blk->getBlockHash();
            if (pbft_sync_period_ + 1 != pbft_blk_and_votes.pbft_blk->getPeriod()) {
              LOG(log_er_pbft_sync_) << "PBFT SYNC ERROR, UNEXPECTED PBFT BLOCK HEIGHT: "
                                     << pbft_blk_and_votes.pbft_blk->getPeriod()
                                     << " sync_period: " << pbft_sync_period_
                                     << " chain size: " << pbft_chain_->getPbftChainSize()
                                     << " queue: " << pbft_chain_->pbftSyncedQueueSize();
              restartSyncingPbft(true);
              return true;
            }
            if (peer->pbft_chain_size_ < pbft_blk_and_votes.pbft_blk->getPeriod()) {
              peer->pbft_chain_size_ = pbft_blk_and_votes.pbft_blk->getPeriod();
            }
            string received_dag_blocks_str;
            map<uint64_t, map<blk_hash_t, pair<DagBlock, vector<Transaction>>>> dag_blocks_per_level;
            for (auto const &dag_blk_struct : pbft_blk_tuple[1]) {
              DagBlock dag_blk(dag_blk_struct[0]);
              auto const &dag_blk_h = dag_blk.getHash();
              peer->markBlockAsKnown(dag_blk_h);
              vector<Transaction> newTransactions;
              for (auto const &trx_raw : dag_blk_struct[1]) {
                auto &trx = newTransactions.emplace_back(trx_raw);
                peer->markTransactionAsKnown(trx.getHash());
              }
              received_dag_blocks_str += dag_blk_h.toString() + " ";
              auto level = dag_blk.getLevel();
              dag_blocks_per_level[level][dag_blk_h] = {move(dag_blk), move(newTransactions)};
            }
            LOG(log_nf_dag_sync_) << "Received Dag Blocks: " << received_dag_blocks_str;
            for (auto const &block_level : dag_blocks_per_level) {
              for (auto const &block : block_level.second) {
                auto status = checkDagBlockValidation(block.second.first);
                if (!status.first) {
                  LOG(log_er_pbft_sync_) << "PBFT SYNC ERROR, DAG missing a tip/pivot: "
                                         << pbft_blk_and_votes.pbft_blk->getPeriod()
                                         << " sync_period: " << pbft_sync_period_
                                         << " chain size: " << pbft_chain_->getPbftChainSize()
                                         << " queue: " << pbft_chain_->pbftSyncedQueueSize();
                  restartSyncingPbft(true);
                  return true;
                }
                LOG(log_nf_dag_sync_) << "Storing block " << block.second.first.getHash().toString() << " with "
                                      << block.second.second.size() << " transactions";
                if (block.second.first.getLevel() > peer->dag_level_) peer->dag_level_ = block.second.first.getLevel();
                dag_blk_mgr_->insertBroadcastedBlockWithTransactions(block.second.first, block.second.second);
              }
            }

            auto pbft_blk_hash = pbft_blk_and_votes.pbft_blk->getBlockHash();
            peer->markPbftBlockAsKnown(pbft_blk_hash);

            // Check the PBFT block whether in the chain or in the synced
            // queue
            if (!pbft_chain_->isKnownPbftBlockForSyncing(pbft_blk_hash)) {
              // Check the PBFT block validation
              if (pbft_chain_->checkPbftBlockValidationFromSyncing(*pbft_blk_and_votes.pbft_blk)) {
                // Notice: cannot verify 2t+1 cert votes here. Since don't
                // have correct account status for nodes which after the
                // first synced one.
                pbft_chain_->setSyncedPbftBlockIntoQueue(pbft_blk_and_votes);
                pbft_sync_period_ = pbft_chain_->pbftSyncingPeriod();
                LOG(log_dg_pbft_sync_) << "Receive synced PBFT block " << pbft_blk_and_votes;
              } else {
                LOG(log_wr_pbft_sync_) << "The PBFT block " << pbft_blk_hash << " failed validation. Drop it!";
              }
            }
          }
          if (pbft_blk_count > 0) {
            if (syncing_ && peer_syncing_pbft_ == _nodeID) {
              if (pbft_sync_period_ > pbft_chain_->getPbftChainSize() + (10 * conf_.network_sync_level_size)) {
                LOG(log_dg_pbft_sync_) << "Syncing pbft blocks faster than processing " << pbft_sync_period_ << " "
                                       << pbft_chain_->getPbftChainSize();
                host_.scheduleExecution(1000, [this, _nodeID]() { delayedPbftSync(_nodeID, 1); });
              } else {
                syncPeerPbft(_nodeID, pbft_sync_period_ + 1);
              }
            } else {
              LOG(log_dg_pbft_sync_) << "Received PbftBlockPacket from node " << _nodeID
                                     << " but currently syncing with peer " << peer_syncing_pbft_;
            }
          } else {
            LOG(log_dg_pbft_sync_) << "Syncing PBFT is completed";
            // We are pbft synced with the node we are connected to but
            // calling restartSyncingPbft will check if some nodes have
            // greater pbft chain size and we should continue syncing with
            // them
            restartSyncingPbft(true);
            // We are pbft synced, send message to other node to start
            // gossiping new blocks
            if (!syncing_) {
              sendSyncedMessage();
            }
          }
          break;
        }
        case TestPacket:
          LOG(log_dg_) << "Received TestPacket";
          ++cnt_received_messages_[_nodeID];
          test_sums_[_nodeID] += _r[0].toInt();
          BOOST_ASSERT(_id == TestPacket);
          return (_id == TestPacket);
      };
    }
    return true;
  } catch (InvalidDataException &e) {
    LOG(log_er_) << "IvalidDataExcecption - Node will be disconnected " << e.what();
  }
  return false;
}

void TaraxaCapability::delayedPbftSync(NodeID _nodeID, int counter) {
  if (!stopped_) {
    if (counter > 60) {
      LOG(log_er_pbft_sync_) << "Pbft blocks stuck in queue, no new block processed "
                                "in 60 seconds "
                             << pbft_sync_period_ << " " << pbft_chain_->getPbftChainSize();
      syncing_ = false;
      LOG(log_dg_pbft_sync_) << "Syncing PBFT is stopping";
      return;
    }
    if (syncing_ && peer_syncing_pbft_ == _nodeID) {
      if (pbft_sync_period_ > pbft_chain_->getPbftChainSize() + (10 * conf_.network_sync_level_size)) {
        LOG(log_dg_pbft_sync_) << "Syncing pbft blocks faster than processing " << pbft_sync_period_ << " "
                               << pbft_chain_->getPbftChainSize();
        host_.scheduleExecution(1000, [this, _nodeID, counter]() { delayedPbftSync(_nodeID, counter + 1); });
      } else {
        syncPeerPbft(_nodeID, pbft_sync_period_ + 1);
      }
    }
  }
}

void TaraxaCapability::restartSyncingPbft(bool force) {
  if (stopped_) return;
  if (syncing_ && !force) {
    LOG(log_dg_pbft_sync_) << "restartSyncingPbft called but syncing_ already true";
    return;
  }

  NodeID max_pbft_chain_nodeID;
  uint64_t max_pbft_chain_size = 0;
  uint64_t max_node_dag_level = 0;
  {
    boost::shared_lock<boost::shared_mutex> lock(peers_mutex_);
    for (auto const peer : peers_) {
      if (peer.second->pbft_chain_size_ > max_pbft_chain_size) {
        max_pbft_chain_size = peer.second->pbft_chain_size_;
        max_pbft_chain_nodeID = peer.first;
        max_node_dag_level = peer.second->dag_level_;
      } else if (peer.second->pbft_chain_size_ == max_pbft_chain_size && peer.second->dag_level_ > max_node_dag_level) {
        max_pbft_chain_nodeID = peer.first;
        max_node_dag_level = peer.second->dag_level_;
      }
    }
  }
  pbft_sync_period_ = pbft_chain_->pbftSyncingPeriod();

  if (max_pbft_chain_size > pbft_sync_period_) {
    if (!stopped_) {
      LOG(log_si_pbft_sync_) << "Restarting syncing PBFT" << max_pbft_chain_size << " " << pbft_sync_period_;
      requesting_pending_dag_blocks_ = false;
      syncing_ = true;
      peer_syncing_pbft_ = max_pbft_chain_nodeID;
      syncPeerPbft(peer_syncing_pbft_, pbft_sync_period_ + 1);
    }
  } else {
    LOG(log_nf_pbft_sync_) << "Restarting syncing PBFT not needed since our pbft chain "
                              "size: "
                           << pbft_sync_period_ << "(" << pbft_chain_->getPbftChainSize() << ")"
                           << " is greater or equal than max node pbft chain size:" << max_pbft_chain_size;
    syncing_ = false;
    if (force || (!requesting_pending_dag_blocks_ &&
                  max_node_dag_level > std::max(dag_mgr_->getMaxLevel(), dag_blk_mgr_->getMaxDagLevelInQueue()))) {
      LOG(log_nf_dag_sync_) << "Request pending " << max_node_dag_level << " "
                            << std::max(dag_mgr_->getMaxLevel(), dag_blk_mgr_->getMaxDagLevelInQueue()) << "("
                            << dag_mgr_->getMaxLevel() << ")";
      requesting_pending_dag_blocks_ = true;
      requesting_pending_dag_blocks_node_id_ = max_pbft_chain_nodeID;
      requestPendingDagBlocks(max_pbft_chain_nodeID);
    }
  }
}

void TaraxaCapability::onDisconnect(NodeID const &_nodeID) {
  LOG(log_nf_) << "Node " << _nodeID << " disconnected";
  cnt_received_messages_.erase(_nodeID);
  test_sums_.erase(_nodeID);
  erasePeer(_nodeID);
  if (syncing_ && peer_syncing_pbft_ == _nodeID && getPeersCount() > 0) {
    LOG(log_dg_pbft_sync_) << "Syncing PBFT is stopping";
    restartSyncingPbft(true);
  }
  if (requesting_pending_dag_blocks_ && requesting_pending_dag_blocks_node_id_ == _nodeID) {
    requesting_pending_dag_blocks_ = false;
    restartSyncingPbft(true);
  }
}

void TaraxaCapability::sendTestMessage(NodeID const &_id, int _x) {
  RLPStream s;
  sealAndSend(_id, host_.capabilityHost()->prep(_id, name(), s, TestPacket, 1) << _x, TestPacket);
}

void TaraxaCapability::sendStatus(NodeID const &_id, bool _initial) {
  RLPStream s;
  if (dag_mgr_) {
    if (_initial) {
      LOG(log_dg_) << "Sending initial status message to " << _id << ", protocol version "
                   << FullNode::c_network_protocol_version << ", network id " << conf_.network_id << ", DAG level "
                   << dag_mgr_->getMaxLevel() << ", genesis " << genesis_ << ", pbft chain size "
                   << pbft_chain_->getPbftChainSize();
    }

    auto dag_max_level = dag_mgr_->getMaxLevel();
    auto pbft_chain_size = pbft_chain_->getPbftChainSize();
    auto pbft_round = pbft_mgr_->getPbftRound();
    auto pbft_previous_round_next_votes_size = next_votes_mgr_->getNextVotesSize();
    LOG(log_dg_dag_sync_) << "Sending status message to " << _id << " with dag level: " << dag_max_level;
    LOG(log_dg_pbft_sync_) << "Sending status message to " << _id << " with pbft chain size: " << pbft_chain_size;
    LOG(log_dg_next_votes_sync_) << "Sending status message to " << _id << " with PBFT round: " << pbft_round
                                 << ", previous round next votes size " << pbft_previous_round_next_votes_size;

    if (_initial) {
      RLPStream rlp = host_.capabilityHost()->prep(_id, name(), s, StatusPacket, 10)
                      << FullNode::c_network_protocol_version << conf_.network_id << dag_max_level << genesis_
                      << pbft_chain_size << syncing_ << pbft_round << pbft_previous_round_next_votes_size
                      << FullNode::c_node_major_version << FullNode::c_node_minor_version;

      LOG(log_dg_) << "RANDOM_TAG [Node " << host_.id() << "] send initial StatusPacket to [Node " << _id << "]: "
                   << "bytes: " << rlp.out() << ", data: " << FullNode::c_network_protocol_version << ", "
                   << conf_.network_id << ", " << dag_max_level << ", " << genesis_ << ", " << pbft_chain_size << ", "
                   << syncing_ << ", " << pbft_round << ", " << pbft_previous_round_next_votes_size << ", "
                   << FullNode::c_node_major_version << ", " << FullNode::c_node_minor_version;

      sealAndSend(_id, rlp, StatusPacket);
    } else {
      RLPStream rlp = host_.capabilityHost()->prep(_id, name(), s, StatusPacket, 5)
                      << dag_max_level << pbft_chain_size << syncing_ << pbft_round
                      << pbft_previous_round_next_votes_size;

      LOG(log_dg_) << "RANDOM_TAG [Node " << host_.id() << "] send standard StatusPacket to [Node " << _id << "]: "
                   << "bytes: " << rlp.out() << ", data: " << dag_max_level << ", " << pbft_chain_size << ", "
                   << syncing_ << ", " << pbft_round << ", " << pbft_previous_round_next_votes_size;

      sealAndSend(_id, rlp, StatusPacket);
    }
  }
}

vector<NodeID> TaraxaCapability::selectPeers(std::function<bool(TaraxaPeer const &)> const &_predicate) {
  vector<NodeID> allowed;
  boost::shared_lock<boost::shared_mutex> lock(peers_mutex_);
  for (auto const &peer : peers_) {
    if (_predicate(*peer.second)) allowed.push_back(peer.first);
  }
  return allowed;
}

vector<NodeID> TaraxaCapability::getAllPeers() const {
  vector<NodeID> peers;
  boost::shared_lock<boost::shared_mutex> lock(peers_mutex_);
  std::transform(
      peers_.begin(), peers_.end(), std::back_inserter(peers),
      [](std::pair<const dev::p2p::NodeID, std::shared_ptr<taraxa::TaraxaPeer>> const &peer) { return peer.first; });
  return peers;
}

std::pair<std::vector<NodeID>, std::vector<NodeID>> TaraxaCapability::randomPartitionPeers(
    std::vector<NodeID> const &_peers, std::size_t _number) {
  vector<NodeID> part1(_peers);
  vector<NodeID> part2;

  if (_number >= _peers.size()) return std::make_pair(part1, part2);

  std::shuffle(part1.begin(), part1.end(), urng_);

  // Remove elements from the end of the shuffled part1 vector and move
  // them to part2.
  std::move(part1.begin() + _number, part1.end(), std::back_inserter(part2));
  part1.erase(part1.begin() + _number, part1.end());
  return std::make_pair(move(part1), move(part2));
}

void TaraxaCapability::onNewTransactions(std::vector<taraxa::bytes> const &transactions, bool fromNetwork) {
  if (fromNetwork) {
    if (dag_blk_mgr_) {
      LOG(log_nf_trx_prp_) << "Storing " << transactions.size() << " transactions";
      received_trx_count += transactions.size();
      unique_received_trx_count += trx_mgr_->insertBroadcastedTransactions(transactions);
    } else {
      for (auto const &transaction : transactions) {
        Transaction trx(transaction);
        auto trx_hash = trx.getHash();
        if (test_transactions_.find(trx_hash) == test_transactions_.end()) {
          test_transactions_[trx_hash] = trx;
          LOG(log_dg_trx_prp_) << "Received New Transaction " << trx_hash;
        } else {
          LOG(log_dg_trx_prp_) << "Received New Transaction" << trx_hash << "that is already known";
        }
      }
    }
  }
  if (!fromNetwork || conf_.network_transaction_interval == 0) {
    std::map<NodeID, std::vector<taraxa::bytes>> transactionsToSend;
    std::map<NodeID, std::vector<trx_hash_t>> transactionsHashToSend;
    {
      boost::unique_lock<boost::shared_mutex> lock(peers_mutex_);
      for (auto &peer : peers_) {
        if (!peer.second->syncing_) {
          for (auto const &transaction : transactions) {
            Transaction trx(transaction);
            auto trx_hash = trx.getHash();
            if (!peer.second->isTransactionKnown(trx_hash)) {
              transactionsToSend[peer.first].push_back(transaction);
              transactionsHashToSend[peer.first].push_back(trx_hash);
            }
          }
        }
      }
    }
    for (auto &it : transactionsToSend) {
      sendTransactions(it.first, it.second);
    }
    boost::unique_lock<boost::shared_mutex> lock(peers_mutex_);
    for (auto &it : transactionsHashToSend) {
      for (auto &it2 : it.second) {
        peers_[it.first]->markTransactionAsKnown(it2);
      }
    }
  }
}

void TaraxaCapability::onNewBlockReceived(DagBlock block, std::vector<Transaction> transactions) {
  LOG(log_nf_dag_prp_) << "Receive DagBlock " << block.getHash() << " #Trx" << transactions.size() << std::endl;
  if (dag_blk_mgr_) {
    auto status = checkDagBlockValidation(block);
    if (!status.first) {
      if (!syncing_ && !requesting_pending_dag_blocks_) restartSyncingPbft();
      return;
    }
    LOG(log_nf_dag_prp_) << "Storing block " << block.getHash().toString() << " with " << transactions.size()
                         << " transactions";
    dag_blk_mgr_->insertBroadcastedBlockWithTransactions(block, transactions);

  } else if (test_blocks_.find(block.getHash()) == test_blocks_.end()) {
    test_blocks_[block.getHash()] = block;
    for (auto tr : transactions) {
      test_transactions_[tr.getHash()] = tr;
    }
    onNewBlockVerified(block);

  } else {
    LOG(log_dg_dag_prp_) << "Received NewBlock " << block.getHash().toString() << "that is already known";
    return;
  }
}

void TaraxaCapability::sendSyncedMessage() {
  LOG(log_dg_dag_sync_) << "sendSyncedMessage ";
  for (auto &peer : getAllPeers()) {
    RLPStream s;
    host_.capabilityHost()->prep(peer, name(), s, SyncedPacket, 0);
    sealAndSend(peer, s, SyncedPacket);
  }
}

void TaraxaCapability::onNewBlockVerified(DagBlock const &block) {
  LOG(log_dg_dag_prp_) << "Verified NewBlock " << block.getHash().toString();
  verified_blocks_.insert(block.getHash());
  {
    std::unique_lock<std::mutex> lck(mtx_for_verified_blocks);
    condition_for_verified_blocks_.notify_all();
  }
  auto const peersWithoutBlock =
      selectPeers([&](TaraxaPeer const &_peer) { return !_peer.isBlockKnown(block.getHash()); });

  auto const peersToSendNumber =
      std::min<std::size_t>(std::max<std::size_t>(conf_.network_min_dag_block_broadcast, std::sqrt(getPeersCount())),
                            conf_.network_max_dag_block_broadcast);

  std::vector<NodeID> peersToSend;
  std::vector<NodeID> peersToAnnounce;
  std::tie(peersToSend, peersToAnnounce) = randomPartitionPeers(peersWithoutBlock, peersToSendNumber);

  for (NodeID const &peerID : peersToSend) {
    RLPStream ts;
    auto peer = getPeer(peerID);
    if (peer && !peer->syncing_) {
      sendBlock(peerID, block);
      peer->markBlockAsKnown(block.getHash());
    }
  }
  if (!peersToSend.empty()) LOG(log_dg_dag_prp_) << "Sent block to" << peersToSend.size() << " peers";

  for (NodeID const &peerID : peersToAnnounce) {
    RLPStream ts;
    auto peer = getPeer(peerID);
    if (peer && !peer->syncing_) {
      sendBlockHash(peerID, block);
      peer->markBlockAsKnown(block.getHash());
    }
  }
  if (!peersToAnnounce.empty()) LOG(log_dg_dag_prp_) << "Anounced block to " << peersToAnnounce.size() << " peers";
}

void TaraxaCapability::sendBlocks(NodeID const &_id, std::vector<std::shared_ptr<DagBlock>> blocks) {
  RLPStream s;
  std::map<blk_hash_t, std::vector<taraxa::bytes>> blockTransactions;
  int totalTransactionsCount = 0;
  for (auto &block : blocks) {
    std::vector<taraxa::bytes> transactions;
    for (auto trx : block->getTrxs()) {
      auto t = trx_mgr_->getTransaction(trx);
      if (!t) {
        if (!stopped_) {
          LOG(log_er_dag_sync_) << "Transacation " << trx << " is not available. SendBlocks canceled";
        }
        // TODO: This can happen on stopping the node because network
        // is not stopped since network does not support restart,
        // better solution needed
        return;
      }
      transactions.push_back(t->second);
      totalTransactionsCount++;
    }
    blockTransactions[block->getHash()] = transactions;
    LOG(log_nf_dag_sync_) << "Send DagBlock " << block->getHash() << "# Trx: " << transactions.size() << std::endl;
  }

  host_.capabilityHost()->prep(_id, name(), s, BlocksPacket, blocks.size() + totalTransactionsCount);
  for (auto &block : blocks) {
    s.appendRaw(block->rlp(true));
    taraxa::bytes trx_bytes;
    for (auto &trx : blockTransactions[block->getHash()]) {
      trx_bytes.insert(trx_bytes.end(), std::begin(trx), std::end(trx));
    }
    s.appendRaw(trx_bytes, blockTransactions[block->getHash()].size());
  }
  sealAndSend(_id, s, BlocksPacket);
}

void TaraxaCapability::sendTransactions(NodeID const &_id, std::vector<taraxa::bytes> const &transactions) {
  LOG(log_nf_trx_prp_) << "sendTransactions" << transactions.size() << " to " << _id;
  RLPStream s;
  host_.capabilityHost()->prep(_id, name(), s, TransactionPacket, transactions.size());
  taraxa::bytes trx_bytes;
  for (auto transaction : transactions) {
    trx_bytes.insert(trx_bytes.end(), std::begin(transaction), std::end(transaction));
  }
  s.appendRaw(trx_bytes, transactions.size());
  sealAndSend(_id, s, TransactionPacket);
}

void TaraxaCapability::sendBlock(NodeID const &_id, taraxa::DagBlock block) {
  RLPStream s;
  vec_trx_t transactionsToSend;
  for (auto trx : block.getTrxs()) {
    auto peer = getPeer(_id);
    if (peer && !peer->isTransactionKnown(trx)) transactionsToSend.push_back(trx);
  }
  host_.capabilityHost()->prep(_id, name(), s, NewBlockPacket, 1 + transactionsToSend.size());
  s.appendRaw(block.rlp(true));

  taraxa::bytes trx_bytes;
  for (auto trx : transactionsToSend) {
    std::shared_ptr<std::pair<Transaction, taraxa::bytes>> transaction;
    if (dag_blk_mgr_) {
      transaction = trx_mgr_->getTransaction(trx);
    } else {
      assert(test_transactions_.find(trx) != test_transactions_.end());
      transaction = std::make_shared<std::pair<Transaction, taraxa::bytes>>(test_transactions_[trx],
                                                                            *test_transactions_[trx].rlp());
    }
    assert(transaction != nullptr);  // We should never try to send a block for
                                     // which  we do not have all transactions
    trx_bytes.insert(trx_bytes.end(), std::begin(transaction->second), std::end(transaction->second));
  }
  s.appendRaw(trx_bytes, transactionsToSend.size());
  sealAndSend(_id, s, NewBlockPacket);
  LOG(log_dg_dag_prp_) << "Send DagBlock " << block.getHash() << " #Trx: " << transactionsToSend.size() << std::endl;
}

void TaraxaCapability::sendBlockHash(NodeID const &_id, taraxa::DagBlock block) {
  LOG(log_dg_dag_prp_) << "sendBlockHash " << block.getHash().toString();
  RLPStream s;
  host_.capabilityHost()->prep(_id, name(), s, NewBlockHashPacket, 1);
  s.append(block.getHash());
  sealAndSend(_id, s, NewBlockHashPacket);
}

void TaraxaCapability::requestBlock(NodeID const &_id, blk_hash_t hash) {
  LOG(log_dg_dag_prp_) << "requestBlock " << hash.toString();
  RLPStream s;
  host_.capabilityHost()->prep(_id, name(), s, GetNewBlockPacket, 1);
  s.append(hash);
  sealAndSend(_id, s, GetNewBlockPacket);
}

void TaraxaCapability::requestPbftBlocks(NodeID const &_id, size_t height_to_sync) {
  RLPStream s;
  host_.capabilityHost()->prep(_id, name(), s, GetPbftBlockPacket, 1);
  s << height_to_sync;
  LOG(log_dg_pbft_sync_) << "Sending GetPbftBlockPacket with height: " << height_to_sync;
  sealAndSend(_id, s, GetPbftBlockPacket);
}

void TaraxaCapability::requestPendingDagBlocks(NodeID const &_id) {
  RLPStream s;
  host_.capabilityHost()->prep(_id, name(), s, GetBlocksPacket, 0);
  LOG(log_nf_dag_sync_) << "Sending GetBlocksPacket";
  sealAndSend(_id, s, GetBlocksPacket);
}

std::pair<int, int> TaraxaCapability::retrieveTestData(NodeID const &_id) {
  int cnt = 0;
  int checksum = 0;
  for (auto i : cnt_received_messages_)
    if (_id == i.first) {
      cnt += i.second;
      checksum += test_sums_[_id];
    }

  return {cnt, checksum};
}

std::map<blk_hash_t, taraxa::DagBlock> TaraxaCapability::getBlocks() { return test_blocks_; }

std::map<trx_hash_t, taraxa::Transaction> TaraxaCapability::getTransactions() { return test_transactions_; }

void TaraxaCapability::sendTransactions() {
  if (trx_mgr_) {
    onNewTransactions(trx_mgr_->getNewVerifiedTrxSnapShotSerialized(), false);
    host_.scheduleExecution(conf_.network_transaction_interval, [this]() { sendTransactions(); });
  }
}

void TaraxaCapability::doBackgroundWork() {
  LOG(log_dg_) << "Periodic status check/send invoked. Num of registered peers: " << peers_.size();

  for (auto const &peer : peers_) {
    // Disconnect any node that did not send any message for 3 status intervals
    if (!peer.second->checkStatus(5)) {
      LOG(log_nf_) << "Host disconnected, no status message in " << 5 * check_status_interval_ << " ms" << peer.first;
      host_.capabilityHost()->disconnect(peer.first, p2p::PingTimeout);
    }
    // Send status message
    else {
      sendStatus(peer.first, false);
    }
  }
  host_.scheduleExecution(check_status_interval_, [this]() { doBackgroundWork(); });
}

void TaraxaCapability::logNetPerformanceStats() {
  LOG(log_nf_net_per_) << "Sent packets stats: " << perf_sent_packets_stats_;
  LOG(log_nf_net_per_) << "Received packets stats: " << perf_received_packets_stats_;

  host_.scheduleExecution(conf_.network_performance_log_interval, [this]() { logNetPerformanceStats(); });

  perf_sent_packets_stats_.clearData();
  perf_received_packets_stats_.clearData();
}

void TaraxaCapability::onStarting() {
  if (conf_.network_simulated_delay > 0) {
    const int number_of_delayed_threads = 5;
    io_work_ = std::make_shared<boost::asio::io_service::work>(io_service_);
    for (int i = 0; i < number_of_delayed_threads; ++i) delay_threads_.create_thread([&]() { io_service_.run(); });
  }

  if (conf_.network_transaction_interval > 0)
    host_.scheduleExecution(conf_.network_transaction_interval, [this]() { sendTransactions(); });

  check_status_interval_ = 6 * lambda_ms_min_;
  host_.scheduleExecution(check_status_interval_, [this]() { doBackgroundWork(); });

  if (conf_.network_performance_log_interval && conf_.network_performance_log) {
    host_.scheduleExecution(conf_.network_performance_log_interval, [this]() { logNetPerformanceStats(); });
  }
}

void TaraxaCapability::onNewPbftVote(taraxa::Vote const &vote) {
  std::vector<NodeID> peers_to_send;
  {
    boost::shared_lock<boost::shared_mutex> lock(peers_mutex_);
    for (auto const &peer : peers_) {
      if (!peer.second->isVoteKnown(vote.getHash())) {
        peers_to_send.push_back(peer.first);
      }
    }
  }
  for (auto const &peer : peers_to_send) {
    sendPbftVote(peer, vote);
  }
}

void TaraxaCapability::sendPbftVote(NodeID const &_id, taraxa::Vote const &vote) {
  LOG(log_dg_vote_prp_) << "sendPbftVote " << vote.getHash() << " to " << _id;
  auto vote_rlp = vote.rlp();

  RLPStream s;
  host_.capabilityHost()->prep(_id, name(), s, PbftVotePacket, 1);
  s.append(vote_rlp);
  sealAndSend(_id, s, PbftVotePacket);
}

void TaraxaCapability::onNewPbftBlock(taraxa::PbftBlock const &pbft_block) {
  std::vector<NodeID> peers_to_send;
  auto my_chain_size = pbft_chain_->getPbftChainSize();
  {
    boost::shared_lock<boost::shared_mutex> lock(peers_mutex_);
    for (auto const &peer : peers_) {
      if (!peer.second->isPbftBlockKnown(pbft_block.getBlockHash())) {
        peers_to_send.push_back(peer.first);
      }
    }
  }
  for (auto const &peer : peers_to_send) {
    sendPbftBlock(peer, pbft_block, my_chain_size);
  }
}

// api for pbft syncing
void TaraxaCapability::sendPbftBlocks(NodeID const &_id, size_t height_to_sync, size_t blocks_to_transfer) {
  LOG(log_dg_pbft_sync_) << "In sendPbftBlocks, peer want to sync from pbft chain height " << height_to_sync
                         << ", will send at most " << blocks_to_transfer << " pbft blocks to " << _id;
  auto pbft_cert_blks = pbft_chain_->getPbftBlocks(height_to_sync, blocks_to_transfer);
  RLPStream s;
  host_.capabilityHost()->prep(_id, name(), s, PbftBlockPacket, pbft_cert_blks.size());
  if (pbft_cert_blks.empty()) {
    sealAndSend(_id, s, PbftBlockPacket);
    LOG(log_dg_pbft_sync_) << "In sendPbftBlocks, sent no pbft blocks to " << _id;
    return;
  }

  // Example actual structure:
  // pbft_blk_1 -> [dag_blk_1, dag_blk_2]
  // pbft_blk_2 -> [dag_blk_3]
  // dag_blk_1 -> [trx_1, trx_2, trx_3]
  // dag_blk_2 -> [trx_4, trx_5, trx_6]
  // dag_blk_3 -> [trx_7, trx_8]
  //
  // Represented in the following variables:
  // level_0 = [pbft_cert_blk_1, pbft_cert_blk_2]
  // level_0_extra = [pbft_blk_1_dag_blk_hashes, pbft_blk_2_dag_blk_hashes]
  // edges_0_to_1 = [0, 2, 3]
  // level_1 = [dag_blk_1, dag_blk_2, dag_blk_3]
  // edges_1_to_2 = [0, 3, 6, 8]
  // level_2 = [trx_1, trx_2, trx_3, trx_4, trx_5, trx_6, trx_7, trx_8]
  //
  // General idea:
  // level_`k`[i] is parent of level_`k+1` elements with ordinals in range from (inclusive) edges_`k`_to_`k+1`[i] to
  // (exclusive) edges_`k`_to_`k+1`[i+1]

  DbStorage::MultiGetQuery db_query(db_);
  auto const &level_0 = pbft_cert_blks;
  for (auto const &b : level_0) {
    db_query.append(DbStorage::Columns::dag_finalized_blocks, b.pbft_blk->getPivotDagBlockHash(), false);
  }
  auto level_0_extra = db_query.execute();
  vector<uint> edges_0_to_1;
  edges_0_to_1.reserve(1 + level_0.size());
  edges_0_to_1.push_back(0);
  for (uint i_0 = 0; i_0 < level_0.size(); ++i_0) {
    db_query.append(DbStorage::Columns::dag_blocks, RLP(level_0_extra[i_0]).toVector<h256>());
    edges_0_to_1.push_back(db_query.size());
  }
  auto level_1 = db_query.execute();
  vector<uint> edges_1_to_2;
  edges_1_to_2.reserve(1 + level_1.size());
  edges_1_to_2.push_back(0);
  for (auto const &dag_blk_raw : level_1) {
    db_query.append(DbStorage::Columns::transactions, DagBlock::extract_transactions_from_rlp(RLP(dag_blk_raw)));
    edges_1_to_2.push_back(db_query.size());
  }
  auto level_2 = db_query.execute();
  for (uint i_0 = 0; i_0 < level_0.size(); ++i_0) {
    s.appendList(2);
    s.appendRaw(level_0[i_0].rlp());
    auto start_1 = edges_0_to_1[i_0];
    auto end_1 = edges_0_to_1[i_0 + 1];
    s.appendList(end_1 - start_1);
    for (uint i_1 = start_1; i_1 < end_1; ++i_1) {
      s.appendList(2);
      s.appendRaw(level_1[i_1]);
      auto start_2 = edges_1_to_2[i_1];
      auto end_2 = edges_1_to_2[i_1 + 1];
      s.appendList(end_2 - start_2);
      for (uint i_2 = start_2; i_2 < end_2; ++i_2) {
        s.appendRaw(level_2[i_2]);
      }
    }
  }
  sealAndSend(_id, s, PbftBlockPacket);
  // Question: will send multiple times to a same receiver, why?
  LOG(log_dg_pbft_sync_) << "Sending PbftCertBlocks to " << _id;
}

void TaraxaCapability::sendPbftBlock(NodeID const &_id, taraxa::PbftBlock const &pbft_block,
                                     uint64_t const &pbft_chain_size) {
  LOG(log_dg_pbft_prp_) << "sendPbftBlock " << pbft_block.getBlockHash() << " to " << _id;

  RLPStream s;
  host_.capabilityHost()->prep(_id, name(), s, NewPbftBlockPacket, 2);
  pbft_block.streamRLP(s, true);
  s << pbft_chain_size;
  sealAndSend(_id, s, NewPbftBlockPacket);
}

void TaraxaCapability::syncPbftNextVotes(uint64_t const pbft_round, size_t const pbft_previous_round_next_votes_size) {
  if (stopped_) {
    return;
  }

  NodeID peer_node_ID;
  uint64_t peer_max_pbft_round = 1;
  size_t peer_max_previous_round_next_votes_size = 0;
  {
    boost::shared_lock<boost::shared_mutex> lock(peers_mutex_);
    // Find max peer PBFT round
    for (auto const &peer : peers_) {
      if (peer.second->pbft_round_ > peer_max_pbft_round) {
        peer_max_pbft_round = peer.second->pbft_round_;
        peer_node_ID = peer.first;
      }
    }

    if (pbft_round == peer_max_pbft_round) {
      // No peers ahead, find peer PBFT previous round max next votes size
      for (auto const &peer : peers_) {
        if (peer.second->pbft_previous_round_next_votes_size_ > peer_max_previous_round_next_votes_size) {
          peer_max_previous_round_next_votes_size = peer.second->pbft_previous_round_next_votes_size_;
          peer_node_ID = peer.first;
        }
      }
    }
  }

  if (!stopped_ && (pbft_round < peer_max_pbft_round ||
                    (pbft_round == peer_max_pbft_round &&
                     pbft_previous_round_next_votes_size < peer_max_previous_round_next_votes_size))) {
    LOG(log_dg_next_votes_sync_) << "Syncing PBFT next votes. Current PBFT round " << pbft_round
                                 << ", previous round next votes size " << pbft_previous_round_next_votes_size
                                 << ". Peer " << peer_node_ID << " is in PBFT round " << peer_max_pbft_round
                                 << ", previous round next votes size " << peer_max_previous_round_next_votes_size;
    requestPbftNextVotes(peer_node_ID, pbft_round, pbft_previous_round_next_votes_size);
  }
}

void TaraxaCapability::requestPbftNextVotes(NodeID const &peerID, uint64_t const pbft_round,
                                            size_t const pbft_previous_round_next_votes_size) {
  RLPStream s;
  host_.capabilityHost()->prep(peerID, name(), s, GetPbftNextVotes, 2);
  s << pbft_round;
  s << pbft_previous_round_next_votes_size;
  LOG(log_dg_next_votes_sync_) << "Sending GetPbftNextVotes with round " << pbft_round
                               << " previous round next votes size " << pbft_previous_round_next_votes_size;
  sealAndSend(peerID, s, GetPbftNextVotes);
}

void TaraxaCapability::sendPbftNextVotes(NodeID const &peerID) {
  std::vector<Vote> next_votes_bundle = next_votes_mgr_->getNextVotes();
  if (next_votes_bundle.empty()) {
    LOG(log_er_next_votes_sync_) << "There are 0 next votes for previous PBFT round";
    return;
  }
  LOG(log_nf_next_votes_sync_) << "Send out size of " << next_votes_bundle.size() << " PBFT next votes to " << peerID;

  RLPStream s;
  host_.capabilityHost()->prep(peerID, name(), s, PbftNextVotesPacket, next_votes_bundle.size());
  for (auto const &next_vote : next_votes_bundle) {
    s.appendRaw(next_vote.rlp());
    LOG(log_nf_next_votes_sync_) << "Send out next vote " << next_vote.getHash() << " to peer " << peerID;
  }
  sealAndSend(peerID, s, PbftNextVotesPacket);
}

void TaraxaCapability::broadcastPreviousRoundNextVotesBundle() {
  std::vector<NodeID> peers_to_send;
  {
    boost::shared_lock<boost::shared_mutex> lock(peers_mutex_);
    for (auto const &peer : peers_) {
      peers_to_send.push_back(peer.first);
    }
  }
  for (auto const &peer : peers_to_send) {
    sendPbftNextVotes(peer);
  }
}

Json::Value TaraxaCapability::getStatus() const {
  Json::Value res;
  res["synced"] = Json::UInt64(!this->syncing_);
  res["peers"] = Json::Value(Json::arrayValue);
  boost::unique_lock<boost::shared_mutex> lock(peers_mutex_);
  for (auto &peer : peers_) {
    Json::Value peer_status;
    peer_status["node_id"] = peer.first.toString();
    peer_status["dag_level"] = Json::UInt64(peer.second->dag_level_);
    peer_status["pbft_size"] = Json::UInt64(peer.second->pbft_chain_size_);
    peer_status["dag_synced"] = !peer.second->syncing_;
    res["peers"].append(peer_status);
  }
  Json::Value counters;
  for (uint8_t it = 0; it != PacketCount; it++) {
    Json::Value counter;
    auto total = packet_count.find(it)->second;
    counter["total"] = Json::UInt64(total);
    if (total > 0) {
      counter["avg packet size"] = Json::UInt64(packet_size.find(it)->second / total);
      auto unique = unique_packet_count.find(it)->second;
      if (unique > 0) {
        counter["unique"] = Json::UInt64(unique);
        counter["unique %"] = Json::UInt64(unique * 100 / total);
      }
      counters[packetToPacketName(it)] = counter;
    }
  }
  counters["transaction count"] = Json::UInt64(received_trx_count);
  counters["unique transaction count"] = Json::UInt64(unique_received_trx_count);
  if (received_trx_count)
    counters["unique transaction %"] = Json::UInt64(unique_received_trx_count * 100 / received_trx_count);
  res["counters"] = counters;
  return res;
}
