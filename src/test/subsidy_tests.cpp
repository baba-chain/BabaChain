// Copyright (c) 2014-2023 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <validation.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(subsidy_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(block_subsidy_test)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);

    uint32_t nPrevBits;
    int32_t nPrevHeight;
    CAmount nSubsidy;

    // details for block 4249 (subsidy returned will be for block 4250)
    nPrevBits = 0x1c4a47c4;
    nPrevHeight = 4249;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 50000000000ULL);

    // details for block 4249 (subsidy returned will be for block 4250)
    // v20 should make difference for blocks with low diff, regardless of their height
    nPrevBits = 0x1c4a47c4;
    nPrevHeight = 4249;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ true);
    BOOST_CHECK_EQUAL(nSubsidy, 500000000ULL);

    // details for block 4501 (subsidy returned will be for block 4502)
    nPrevBits = 0x1c4a47c4;
    nPrevHeight = 4501;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 5600000000ULL);

    // details for block 5464 (subsidy returned will be for block 5465)
    nPrevBits = 0x1c29ec00;
    nPrevHeight = 5464;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 2100000000ULL);

    // details for block 5465 (subsidy returned will be for block 5466)
    nPrevBits = 0x1c29ec00;
    nPrevHeight = 5465;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 12200000000ULL);

    // details for block 17588 (subsidy returned will be for block 17589)
    nPrevBits = 0x1c08ba34;
    nPrevHeight = 17588;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 6100000000ULL);

    // details for block 99999 (subsidy returned will be for block 100000)
    nPrevBits = 0x1b10cf42;
    nPrevHeight = 99999;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 500000000ULL);

    // details for block 210239 (subsidy returned will be for block 210240)
    nPrevBits = 0x1b11548e;
    nPrevHeight = 210239;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 500000000ULL);

    // 1st subsidy reduction happens here

    // details for block 210240 (subsidy returned will be for block 210241)
    nPrevBits = 0x1b10d50b;
    nPrevHeight = 210240;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 464285715ULL);

    // details for block 210240 (subsidy returned will be for block 210241)
    // v20 makes no difference for blocks with high enough diff while budgets aren't active yet
    nPrevBits = 0x1b10d50b;
    nPrevHeight = 210240;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ true);
    BOOST_CHECK_EQUAL(nSubsidy, 464285715ULL);

    // details for block 420480 (subsidy returned will be for block 210241)
    nPrevBits = 0x1b10d50b;
    nPrevHeight = 420480;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 388010205ULL); // 431122450 * 0.9

    // details for block 420480 (subsidy returned will be for block 210241)
    // budgets are active, reallocation matters now
    nPrevBits = 0x1b10d50b;
    nPrevHeight = 420480;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ true);
    BOOST_CHECK_EQUAL(nSubsidy, 344897960ULL); // 431122450 * 0.8
}

BOOST_AUTO_TEST_CASE(babachain_fixed_reward_test)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    const auto& consensus = chainParams->GetConsensus();
    
    // Test genesis block (should return premine amount)
    CAmount nSubsidy = GetBabaChainPoSSubsidy(0, consensus);
    BOOST_CHECK_EQUAL(nSubsidy, 20000000 * COIN); // 20M premine
    
    // Test initial block reward (should be 200 BabaChain)
    nSubsidy = GetBabaChainPoSSubsidy(1, consensus);
    BOOST_CHECK_EQUAL(nSubsidy, 200 * COIN); // 200 BabaChain initial reward
    
    // Test supply tracking
    CAmount nSupply = GetBabaChainCirculatingSupply(0, consensus);
    BOOST_CHECK_EQUAL(nSupply, 20000000 * COIN); // Genesis supply equals premine
    
    // Test fixed reward system (always 200 BabaChain per block)
    CAmount nReward;
    
    // At 20M supply (premine only) - should get 200 BabaChain
    nReward = CalculateStakingReward(1000, 20000000 * COIN, consensus);
    BOOST_CHECK_EQUAL(nReward, 200 * COIN); // Fixed reward
    
    // At 50M supply - still 200 BabaChain (no reductions)
    nReward = CalculateStakingReward(1000, 50000000 * COIN, consensus);
    BOOST_CHECK_EQUAL(nReward, 200 * COIN); // Fixed reward
    
    // At 100M supply - still 200 BabaChain (no reductions)
    nReward = CalculateStakingReward(1000, 100000000 * COIN, consensus);
    BOOST_CHECK_EQUAL(nReward, 200 * COIN); // Fixed reward
    
    // At 200M supply - still 200 BabaChain (no reductions)
    nReward = CalculateStakingReward(1000, 200000000 * COIN, consensus);
    BOOST_CHECK_EQUAL(nReward, 200 * COIN); // Fixed reward
    
    // Test supply cap enforcement
    bool bSupplyValid = EnforceSupplyCap(1000000, consensus); // Very high block number
    BOOST_CHECK(bSupplyValid); // Should still be valid due to supply cap
    
    // Test individual staking rewards with different stake amounts
    CAmount nTotalNetworkStake = 1000000 * COIN; // 1M total staked
    CAmount nBlockReward = 200 * COIN;
    
    // Test 1,000 BabaChain stake (minimum)
    CAmount nReward1K = CalculateIndividualStakingReward(1000 * COIN, nTotalNetworkStake, nBlockReward, consensus);
    BOOST_CHECK(nReward1K > 0); // Should get some reward
    
    // Test 10,000 BabaChain stake (5% bonus)
    CAmount nReward10K = CalculateIndividualStakingReward(10000 * COIN, nTotalNetworkStake, nBlockReward, consensus);
    BOOST_CHECK(nReward10K > nReward1K * 10); // Should get more than 10x due to bonus
    
    // Test 100,000 BabaChain stake (20% bonus)
    CAmount nReward100K = CalculateIndividualStakingReward(100000 * COIN, nTotalNetworkStake, nBlockReward, consensus);
    BOOST_CHECK(nReward100K > nReward1K * 100); // Should get more than 100x due to bonus
    
    // Test expected daily rewards
    CAmount nDailyReward1K = CalculateExpectedDailyRewards(1000 * COIN, 25000000 * COIN, nTotalNetworkStake, consensus);
    CAmount nDailyReward10K = CalculateExpectedDailyRewards(10000 * COIN, 25000000 * COIN, nTotalNetworkStake, consensus);
    CAmount nDailyReward100K = CalculateExpectedDailyRewards(100000 * COIN, 25000000 * COIN, nTotalNetworkStake, consensus);
    
    // Verify daily rewards are proportional with bonuses
    BOOST_CHECK(nDailyReward10K > nDailyReward1K * 10); // 10K should earn more than 10x
    BOOST_CHECK(nDailyReward100K > nDailyReward1K * 100); // 100K should earn more than 100x
    
    // Test staking requirements validation (very liberal - just 1 BabaChain minimum)
    bool bValidStake = ValidateStakingRequirements(1 * COIN, 8 * 60 * 60, consensus);
    BOOST_CHECK(bValidStake); // Should be valid (just 1 BabaChain!)
    
    bool bInvalidStake = ValidateStakingRequirements(0.5 * COIN, 4 * 60 * 60, consensus);
    BOOST_CHECK(!bInvalidStake); // Should be invalid (below 1 BabaChain or too young)
    
    // Test reward distribution
    std::vector<CAmount> vStakes = {1000 * COIN, 10000 * COIN, 100000 * COIN}; // Different stake sizes
    CAmount nTotalReward = 200 * COIN;
    CAmount nDistributed = DistributeStakingRewards(vStakes, nTotalReward);
    BOOST_CHECK_EQUAL(nDistributed, nTotalReward); // All rewards should be distributed
}

BOOST_AUTO_TEST_SUITE_END()
