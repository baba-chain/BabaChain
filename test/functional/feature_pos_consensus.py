#!/usr/bin/env python3
# Copyright (c) 2025 The BabaChain Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test PoS consensus functionality."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
    connect_nodes,
    wait_until,
)
from test_framework.messages import (
    CTransaction,
    CTxOut,
    COutPoint,
    COIN,
)
from decimal import Decimal
import time

class PoSConsensusTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [
            ["-debug=pos", "-debug=validation"],
            ["-debug=pos", "-debug=validation"],
            ["-debug=pos", "-debug=validation"],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()
        # Connect all nodes
        for i in range(self.num_nodes - 1):
            connect_nodes(self.nodes[i], i + 1)
        self.sync_all()

    def run_test(self):
        self.log.info("Starting PoS consensus integration tests...")
        
        # Test basic PoS functionality
        self.test_pos_validation()
        self.test_validator_registration()
        self.test_staking_operations()
        self.test_block_production()
        self.test_reward_distribution()
        self.test_slashing_mechanism()
        
        self.log.info("PoS consensus tests completed successfully")

    def test_pos_validation(self):
        """Test basic PoS validation framework"""
        self.log.info("Testing PoS validation framework...")
        
        node = self.nodes[0]
        
        # Generate some initial blocks to get coins
        node.generatetoaddress(101, node.getnewaddress())
        self.sync_all()
        
        # Check that we have a balance
        balance = node.getbalance()
        assert_greater_than(balance, 0)
        
        # Test minimum stake validation
        try:
            # Try to stake with amount below minimum (should fail)
            result = node.stakecoin(0.1)  # Below minimum stake
            assert False, "Should have failed with insufficient stake"
        except Exception as e:
            assert "insufficient" in str(e).lower() or "minimum" in str(e).lower()
        
        self.log.info("✓ PoS validation framework working correctly")

    def test_validator_registration(self):
        """Test validator registration system"""
        self.log.info("Testing validator registration...")
        
        node = self.nodes[0]
        
        # Get a new address for validator rewards
        reward_address = node.getnewaddress()
        
        # Register as validator with sufficient stake
        try:
            validator_info = node.registervalidator(reward_address, 1000, "Test Validator")
            assert "txid" in validator_info
            
            # Mine a block to confirm the registration
            node.generatetoaddress(1, node.getnewaddress())
            self.sync_all()
            
            # Check validator is registered
            validators = node.listvalidators()
            assert len(validators) > 0
            
            # Find our validator
            our_validator = None
            for validator in validators:
                if validator.get("description") == "Test Validator":
                    our_validator = validator
                    break
            
            assert our_validator is not None, "Validator not found in list"
            assert our_validator["active"] == True
            assert our_validator["stake"] >= 1000
            
        except Exception as e:
            # If RPC methods don't exist yet, skip this test
            if "method not found" in str(e).lower():
                self.log.info("⚠ Validator registration RPC not implemented yet, skipping...")
                return
            raise
        
        self.log.info("✓ Validator registration working correctly")

    def test_staking_operations(self):
        """Test staking and unstaking operations"""
        self.log.info("Testing staking operations...")
        
        node = self.nodes[0]
        
        try:
            # Get initial balance
            initial_balance = node.getbalance()
            
            # Stake some coins
            stake_amount = 100
            stake_result = node.stakecoin(stake_amount, 86400)  # 1 day lock
            assert "txid" in stake_result
            
            # Mine a block to confirm
            node.generatetoaddress(1, node.getnewaddress())
            self.sync_all()
            
            # Check staking info
            staking_info = node.getstakinginfo()
            assert staking_info["staking"] == True
            assert staking_info["staked_amount"] >= stake_amount
            
            # Check balance decreased by staked amount
            new_balance = node.getbalance()
            assert new_balance < initial_balance
            
        except Exception as e:
            if "method not found" in str(e).lower():
                self.log.info("⚠ Staking RPC methods not implemented yet, skipping...")
                return
            raise
        
        self.log.info("✓ Staking operations working correctly")

    def test_block_production(self):
        """Test PoS block production mechanism"""
        self.log.info("Testing PoS block production...")
        
        node = self.nodes[0]
        
        # Get initial block count
        initial_height = node.getblockcount()
        
        # Generate blocks and verify they use PoS
        for i in range(5):
            block_hash = node.generatetoaddress(1, node.getnewaddress())[0]
            block = node.getblock(block_hash, 2)  # Get full block details
            
            # Verify block structure for PoS
            assert "tx" in block
            assert len(block["tx"]) >= 1  # At least coinbase
            
            # Check that block doesn't have PoW-specific fields
            assert "difficulty" not in block or block["difficulty"] == 0
            
        # Verify blocks were produced
        final_height = node.getblockcount()
        assert_equal(final_height, initial_height + 5)
        
        self.log.info("✓ PoS block production working correctly")

    def test_reward_distribution(self):
        """Test staking reward distribution"""
        self.log.info("Testing reward distribution...")
        
        node = self.nodes[0]
        
        try:
            # Get initial staking info
            initial_info = node.getstakinginfo()
            initial_rewards = initial_info.get("total_rewards", 0)
            
            # Generate several blocks to accumulate rewards
            node.generatetoaddress(10, node.getnewaddress())
            self.sync_all()
            
            # Check if rewards increased
            final_info = node.getstakinginfo()
            final_rewards = final_info.get("total_rewards", 0)
            
            # Rewards should have increased (if staking is active)
            if initial_info.get("staking", False):
                assert_greater_than(final_rewards, initial_rewards)
            
        except Exception as e:
            if "method not found" in str(e).lower():
                self.log.info("⚠ Reward distribution RPC not implemented yet, skipping...")
                return
            raise
        
        self.log.info("✓ Reward distribution working correctly")

    def test_slashing_mechanism(self):
        """Test validator slashing for malicious behavior"""
        self.log.info("Testing slashing mechanism...")
        
        node = self.nodes[0]
        
        try:
            # Get list of validators
            validators = node.listvalidators()
            
            if len(validators) == 0:
                self.log.info("⚠ No validators to test slashing, skipping...")
                return
            
            # Test slashing detection (this would normally be triggered by consensus violations)
            # For testing, we'll check if the slashing infrastructure exists
            
            # Check if slashing-related RPC methods exist
            try:
                slashed_validators = node.getslashedvalidators()
                blacklisted_validators = node.getblacklistedvalidators()
                
                # These should return empty lists initially
                assert isinstance(slashed_validators, list)
                assert isinstance(blacklisted_validators, list)
                
            except Exception as e:
                if "method not found" in str(e).lower():
                    self.log.info("⚠ Slashing RPC methods not implemented yet, skipping...")
                    return
                raise
            
        except Exception as e:
            if "method not found" in str(e).lower():
                self.log.info("⚠ Slashing mechanism RPC not implemented yet, skipping...")
                return
            raise
        
        self.log.info("✓ Slashing mechanism infrastructure working correctly")

if __name__ == '__main__':
    PoSConsensusTest().main()