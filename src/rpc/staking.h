// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_STAKING_H
#define BITCOIN_RPC_STAKING_H

class CRPCTable;

/** Register staking RPC commands */
void RegisterStakingRPCCommands(CRPCTable &tableRPC);

#endif // BITCOIN_RPC_STAKING_H