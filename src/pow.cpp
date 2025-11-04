// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include <pos.h>

#include <math.h>

// Legacy PoW functions removed for PoS transition
// KimotoGravityWell and DarkGravityWave algorithms are no longer used

unsigned int GetNextPoSTarget(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    // For PoS, we use a simpler target adjustment based on block timing
    // Target adjusts to maintain the desired block spacing
    
    if (!pindexLast) {
        // Return maximum target for genesis block
        return UintToArith256(params.powLimit).GetCompact();
    }
    
    // For the first few blocks, use maximum target
    if (pindexLast->nHeight < 10) {
        return UintToArith256(params.powLimit).GetCompact();
    }
    
    // Calculate average block time over the last 10 blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; i < 9 && pindexFirst->pprev; i++) {
        pindexFirst = pindexFirst->pprev;
    }
    
    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    int64_t nTargetTimespan = 9 * params.nStakeTargetSpacing; // 9 blocks worth of target spacing
    
    // Limit adjustment to prevent wild swings
    if (nActualTimespan < nTargetTimespan / 4) {
        nActualTimespan = nTargetTimespan / 4;
    }
    if (nActualTimespan > nTargetTimespan * 4) {
        nActualTimespan = nTargetTimespan * 4;
    }
    
    // Calculate new target
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;
    
    // Ensure target doesn't exceed the limit
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    if (bnNew > bnPowLimit) {
        bnNew = bnPowLimit;
    }
    
    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    // For PoS, we use the new target calculation method
    return GetNextPoSTarget(pindexLast, params);
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    // For PoS blocks, we don't check traditional proof of work
    // This function is kept for compatibility but always returns true for PoS
    // The actual validation is done through CheckProofOfStake
    return true;
}

bool CheckProofOfStakeTarget(unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    return true;
}
