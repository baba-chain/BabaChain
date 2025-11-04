// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include <consensus/params.h>

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class uint256;

/** Get next target for PoS blocks */
unsigned int GetNextPoSTarget(const CBlockIndex* pindexLast, const Consensus::Params& params);

/** Legacy function for compatibility - now uses PoS target calculation */
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params&);

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits (PoS compatibility) */
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params&);

/** Check whether nBits represents a valid PoS target */
bool CheckProofOfStakeTarget(unsigned int nBits, const Consensus::Params& params);

#endif // BITCOIN_POW_H
