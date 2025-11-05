// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_STAKING_H
#define BITCOIN_WALLET_STAKING_H

#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <uint256.h>
#include <script/script.h>

#include <string>
#include <vector>

namespace wallet {

class CWallet;

/**
 * Information about a staking transaction
 */
struct CStakingInfo {
    uint256 txid;                   // Transaction ID
    int nType;                      // Transaction type (TRANSACTION_STAKE, etc.)
    int64_t nTime;                  // Transaction time
    CAmount nAmount;                // Amount involved
    int nDepth;                     // Confirmation depth
    int64_t nLockTime;              // Lock duration (for stake transactions)
    CPubKey stakePubKey;            // Staking public key
    CPubKey validatorPubKey;        // Validator public key (for validator registration)
    COutPoint stakeOutpoint;        // Referenced stake outpoint (for unstake transactions)
    std::string strDescription;     // Description (for validator registration)
    
    CStakingInfo() : nType(0), nTime(0), nAmount(0), nDepth(0), nLockTime(0) {}
};

/**
 * Information about a validator owned by this wallet
 */
struct CValidatorInfo {
    CPubKey pubkey;                 // Validator public key
    CAmount nStakeAmount;           // Validator stake amount
    std::string strDescription;     // Validator description
    int64_t nRegistrationTime;      // Registration timestamp
    uint256 txid;                   // Registration transaction ID
    bool fActive;                   // Whether validator is active
    bool fSlashed;                  // Whether validator has been slashed
    bool fBlacklisted;              // Whether validator is blacklisted
    
    CValidatorInfo() : nStakeAmount(0), nRegistrationTime(0), fActive(false), fSlashed(false), fBlacklisted(false) {}
};

/**
 * Get all staking transactions from the wallet
 */
std::vector<CStakingInfo> GetStakingTransactions(const CWallet& wallet);

/**
 * Get all validators owned by this wallet
 */
std::vector<CValidatorInfo> GetWalletValidators(const CWallet& wallet);

/**
 * Get total staked balance for this wallet
 */
CAmount GetStakedBalance(const CWallet& wallet);

/**
 * Get balance available for staking (not currently staked)
 */
CAmount GetStakingBalance(const CWallet& wallet);

/**
 * Create a stake transaction
 */
bool CreateStakeTransaction(CWallet& wallet, CAmount nAmount, int64_t nLockTime, 
                           const CTxDestination& dest, CTransactionRef& txOut, std::string& strError);

/**
 * Create an unstake transaction
 */
bool CreateUnstakeTransaction(CWallet& wallet, const COutPoint& stakeOutpoint, 
                             const CTxDestination& dest, CTransactionRef& txOut, std::string& strError);

/**
 * Create a validator registration transaction
 */
bool CreateValidatorRegistration(CWallet& wallet, CAmount nStakeAmount, const CPubKey& validatorPubKey,
                                const std::string& strDescription, const CTxDestination& rewardDest,
                                CTransactionRef& txOut, std::string& strError);

/**
 * Calculate expected staking rewards for a given amount and duration
 */
CAmount CalculateExpectedStakingReward(CAmount nStakeAmount, int64_t nStakeDuration);

/**
 * Check if wallet has any active stakes
 */
bool HasActiveStakes(const CWallet& wallet);

/**
 * Check if wallet has any registered validators
 */
bool HasRegisteredValidators(const CWallet& wallet);

} // namespace wallet

#endif // BITCOIN_WALLET_STAKING_H