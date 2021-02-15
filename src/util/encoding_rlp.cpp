#include "encoding_rlp.hpp"

namespace taraxa::util::encoding_rlp {

// TODO: unable to declare this method elsewhere and make it work...
// void enc_rlp(RLPStream& rlp, const dev::eth::Transaction& obj) {
//  enc_rlp_tuple(rlp, obj.from(), obj.gasPrice(), obj.isCreation() ? dev::bytesConstRef() : obj.to().ref(),
//  obj.nonce(),
//                obj.value(), obj.gas(), obj.data());
//}

void enc_rlp(dev::RLPStream& rlp, const DagStats::TransactionStats& obj) {
  enc_rlp_tuple(rlp, obj.miner_, obj.uncle_miners_);
}

void enc_rlp(dev::RLPStream& rlp, const DagStats::BlocksStats& obj) {
  enc_rlp_tuple(rlp, obj.miners_blocks_count_, obj.total_blocks_count_);
}

}  // namespace taraxa::util::encoding_rlp