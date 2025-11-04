# BabaChain Proof-of-Stake Consensus

## Overview

BabaChain implements a modern Proof-of-Stake (PoS) consensus mechanism that is energy-efficient, secure, and fair. This document provides technical details about the consensus algorithm and its implementation.

## Consensus Algorithm

### Block Production

BabaChain uses a stake-weighted random selection algorithm for block production:

1. **Validator Selection**: Validators are selected based on their stake weight
2. **Block Creation**: Selected validator creates and signs the block
3. **Network Propagation**: Block is broadcast to the network
4. **Validation**: Other nodes validate the block and its transactions
5. **Finalization**: Block is added to the blockchain after confirmations

### Stake Weight Calculation

```
stake_weight = coin_amount × coin_age × bonus_multiplier
```

Where:
- `coin_amount`: Number of coins in the stake
- `coin_age`: Time since coins last moved (minimum 8 hours)
- `bonus_multiplier`: Gradual bonus based on total stake size

### Block Time and Difficulty

- **Target Block Time**: 2.5 minutes (150 seconds)
- **Difficulty Adjustment**: Every block based on recent block times
- **Minimum Difficulty**: Prevents network stalling
- **Maximum Difficulty**: Prevents excessive delays

## Validator System

### Validator Registration

Validators are automatically registered when they:
1. Hold BabaChain coins in their wallet
2. Keep their wallet online and unlocked for staking
3. Have coins that have matured (8+ hours old)
4. Maintain network connectivity

### Validator Selection Algorithm

```python
def select_validator(validators, block_hash):
    """
    Select validator using stake-weighted randomness
    """
    total_weight = sum(v.stake_weight for v in validators)
    target = hash_to_number(block_hash) % total_weight
    
    current_weight = 0
    for validator in validators:
        current_weight += validator.stake_weight
        if current_weight >= target:
            return validator
```

### Validator Responsibilities

1. **Block Production**: Create valid blocks when selected
2. **Transaction Validation**: Verify transaction validity
3. **Network Participation**: Maintain network connectivity
4. **Consensus Participation**: Vote on network upgrades

## Security Mechanisms

### Slashing Conditions

Validators can be penalized for:

1. **Double Signing**: Creating multiple blocks at same height
2. **Invalid Blocks**: Producing blocks with invalid transactions
3. **Network Attacks**: Attempting to manipulate consensus
4. **Long Absence**: Extended periods offline (future feature)

### Slashing Penalties

- **Minor Violations**: 1-5% of stake burned
- **Major Violations**: 10-25% of stake burned
- **Severe Attacks**: 50-100% of stake burned
- **Temporary Ban**: Exclusion from validation for period

### Attack Resistance

**51% Attack Protection**
- Requires controlling 51% of total stake
- Economic disincentive (attacker loses their stake)
- Slashing mechanism punishes malicious behavior
- Community can fork away from attack

**Nothing-at-Stake Attack**
- Coin age requirement prevents costless validation
- Slashing for double-signing
- Checkpoint system for finality

**Long-Range Attack**
- Checkpoint system prevents deep reorganizations
- Social consensus for dispute resolution
- Regular network upgrades

## Economic Model

### Reward Distribution

**Block Rewards**
- Base reward: 1% of validator's stake daily
- Gradual bonus: Up to 20% additional for large stakes
- Network fees: Transaction fees go to block producer
- Total supply cap: 1 billion BabaChain maximum

**Reward Calculation**
```python
def calculate_reward(stake_amount, days_staking):
    base_rate = 0.01  # 1% daily
    bonus_rate = calculate_gradual_bonus(stake_amount)
    daily_reward = stake_amount * (base_rate + bonus_rate)
    return daily_reward
```

### Gradual Bonus System

The bonus system provides smooth progression without harsh tier cutoffs:

```python
def calculate_gradual_bonus(stake_amount):
    if stake_amount <= 10000:
        # 0% to 5% bonus for 1-10K coins
        return (stake_amount / 10000) * 0.05
    elif stake_amount <= 100000:
        # 5% to 20% bonus for 10K-100K coins
        progress = (stake_amount - 10000) / 90000
        return 0.05 + (progress * 0.15)
    else:
        # Maximum 20% bonus for 100K+ coins
        return 0.20
```

### Supply Economics

- **Genesis Supply**: 210 million BabaChain
- **Premine**: 20 million (10% for development/marketing)
- **Staking Pool**: 190 million (90% for community rewards)
- **Extended Pool**: 790 million (future staking rewards)
- **Hard Cap**: 1 billion BabaChain total

## Network Parameters

### Consensus Parameters

```cpp
// Consensus parameters for BabaChain PoS
struct ConsensusParams {
    // Block timing
    int64_t nTargetSpacing = 150;           // 2.5 minutes
    int64_t nStakeMinAge = 8 * 60 * 60;     // 8 hours
    int64_t nStakeMaxAge = -1;              // No maximum age
    
    // Staking parameters
    int64_t nMinStakeAmount = 1 * COIN;     // 1 BabaChain minimum
    int nStakeMinConfirmations = 1;         // 1 confirmation
    
    // Reward parameters
    int64_t nBaseStakeReward = 1000000;     // 1% daily (in basis points)
    int64_t nMaxBonusRate = 2000000;        // 20% max bonus (in basis points)
    
    // Network security
    int nMaxReorgDepth = 100;               // Maximum reorg depth
    int nCheckpointSpan = 1000;             // Checkpoint every 1000 blocks
};
```

### Network Magic and Ports

```cpp
// Network identification
static const uint32_t BABACHAIN_MAINNET_MAGIC = 0xbaba2024;
static const uint32_t BABACHAIN_TESTNET_MAGIC = 0xbaba2025;
static const uint32_t BABACHAIN_REGTEST_MAGIC = 0xbaba2026;

// Default network ports
static const int BABACHAIN_MAINNET_PORT = 9999;
static const int BABACHAIN_TESTNET_PORT = 19999;
static const int BABACHAIN_REGTEST_PORT = 29999;
```

## Implementation Details

### Block Structure

```cpp
class CBlockHeader {
public:
    // Standard block header fields
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;
    
    // PoS specific fields
    uint256 hashStakeModifier;
    COutPoint prevoutStake;
    uint256 hashStakeBlock;
    CScript scriptStakeValidator;
};
```

### Stake Validation

```cpp
bool CheckStakeKernelHash(
    const CBlockIndex* pindexPrev,
    const CTransaction& txPrev,
    const COutPoint& prevout,
    uint32_t nTimeTx,
    uint256& hashProofOfStake
) {
    // Calculate stake kernel hash
    CDataStream ss(SER_GETHASH, 0);
    ss << pindexPrev->GetBlockHash()
       << txPrev.nTime
       << prevout.hash
       << prevout.n
       << nTimeTx;
    
    hashProofOfStake = Hash(ss.begin(), ss.end());
    
    // Check if hash meets difficulty target
    CBigNum bnTarget;
    bnTarget.SetCompact(GetNextTargetRequired(pindexPrev, true));
    
    CBigNum bnWeight = GetStakeWeight(txPrev, nTimeTx);
    bnTarget *= bnWeight;
    
    return CBigNum(hashProofOfStake) <= bnTarget;
}
```

### Validator Management

```cpp
class CValidatorRegistry {
private:
    std::map<COutPoint, CValidator> mapValidators;
    std::set<COutPoint> setActiveValidators;
    
public:
    bool RegisterValidator(const COutPoint& stake, const CValidator& validator);
    bool DeregisterValidator(const COutPoint& stake);
    bool IsValidatorActive(const COutPoint& stake) const;
    std::vector<CValidator> GetActiveValidators() const;
    CValidator SelectValidator(const uint256& blockHash) const;
};
```

## Network Upgrades

### Upgrade Mechanism

1. **Proposal**: Developers propose network upgrade
2. **Signaling**: Validators signal support in blocks
3. **Activation**: Upgrade activates at threshold (75% support)
4. **Enforcement**: New rules enforced after activation

### Upgrade History

- **Genesis**: Initial PoS implementation
- **v1.1**: Gradual bonus system implementation
- **v1.2**: Enhanced slashing mechanism
- **v2.0**: Mobile staking support (planned)

## Testing and Validation

### Unit Tests

```bash
# Run PoS consensus tests
make check TESTS="pos_tests"

# Run validator tests
make check TESTS="validator_tests"

# Run reward calculation tests
make check TESTS="reward_tests"
```

### Integration Tests

```bash
# Test full PoS workflow
test/functional/pos_basic.py

# Test validator selection
test/functional/pos_validator_selection.py

# Test slashing mechanism
test/functional/pos_slashing.py
```

### Performance Benchmarks

```bash
# Benchmark validator selection
src/bench/bench_babachain -filter="ValidatorSelection"

# Benchmark stake weight calculation
src/bench/bench_babachain -filter="StakeWeight"

# Benchmark block validation
src/bench/bench_babachain -filter="BlockValidation"
```

## Future Improvements

### Planned Features

1. **Cold Staking**: Stake from offline wallets
2. **Delegation**: Delegate staking to validators
3. **Governance**: On-chain voting for upgrades
4. **Sharding**: Horizontal scaling solution

### Research Areas

1. **Finality Gadgets**: Faster transaction finality
2. **Cross-Chain**: Interoperability with other chains
3. **Privacy**: Zero-knowledge staking proofs
4. **Scalability**: Layer 2 solutions

---

*This document is maintained by the BabaChain development team. For questions or contributions, please visit our [GitHub repository](https://github.com/baba-chain/babachain).*