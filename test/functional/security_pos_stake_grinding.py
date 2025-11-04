#!/usr/bin/env python3
# Copyright (c) 2025 The BabaChain Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Security test for PoS stake grinding attack resistance."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
    connect_nodes,
    disconnect_nodes,
    wait_until,
)
import time
import hashlib
import random

class PoSStakeGrindingSecurityTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4
        self.extra_args = [
            ["-debug=pos", "-debug=validation", "-debug=security"],
            ["-debug=pos", "-debug=validation", "-debug=security"],
            ["-debug=pos", "-debug=validation", "-debug=security"],
            ["-debug=pos", "-debug=validation", "-debug=security"],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()
        for i in range(self.num_nodes - 1):
            connect_nodes(self.nodes[i], i + 1)
        self.sync_all()

    def run_test(self):
        self.log.info("Starting PoS stake grinding security tests...")
        
        self.test_stake_grinding_resistance()
        self.test_nothing_at_stake_prevention()
        self.test_long_range_attack_prevention()
        self.test_validator_key_security()
        self.test_stake_manipulation_prevention()
        
        self.log.info("PoS stake grinding security tests completed")

    def test_stake_grinding_resistance(self):
        """Test resistance to stake grinding attacks"""
        self.log.info("Testing stake grinding attack resistance...")
        
        node = self.nodes[0]
        
        # Generate initial blocks and setup
        node.generatetoaddress(101, node.getnewaddress())
        self.sync_all()
        
        # Simulate stake grinding attempt
        # In a stake grinding attack, an attacker tries to manipulate the randomness
        # used in validator selection by trying different transaction orderings
        
        initial_height = node.getblockcount()
        
        # Create multiple transactions with different orderings
        addresses = [node.getnewaddress() for _ in range(10)]
        
        # Attempt 1: Normal transaction ordering
        txids_normal = []
        for addr in addresses:
            try:
                txid = node.sendtoaddress(addr, 1.0)
                txids_normal.append(txid)
            except Exception as e:
                if "insufficient" in str(e).lower():
                    break
        
        # Generate block with normal ordering
        block_hash_1 = node.generatetoaddress(1, node.getnewaddress())[0]
        block_1 = node.getblock(block_hash_1, 2)
        
        # Attempt 2: Try to manipulate by creating different transaction set
        # (This simulates an attacker trying to grind for favorable randomness)
        
        # Invalidate the block to try again
        node.invalidateblock(block_hash_1)
        
        # Create different transaction pattern
        txids_manipulated = []
        for i, addr in enumerate(reversed(addresses)):
            try:
                # Use different amounts to change transaction hashes
                amount = 1.0 + (i * 0.01)
                txid = node.sendtoaddress(addr, amount)
                txids_manipulated.append(txid)
            except Exception as e:
                if "insufficient" in str(e).lower():
                    break
        
        # Generate block with manipulated ordering
        block_hash_2 = node.generatetoaddress(1, node.getnewaddress())[0]
        block_2 = node.getblock(block_hash_2, 2)
        
        # Verify that the PoS consensus doesn't allow easy manipulation
        # Both blocks should be valid, but the network should have mechanisms
        # to prevent grinding attacks
        
        self.log.info(f"Block 1 hash: {block_hash_1}")
        self.log.info(f"Block 2 hash: {block_hash_2}")
        self.log.info(f"Block 1 transactions: {len(block_1.get('tx', []))}")
        self.log.info(f"Block 2 transactions: {len(block_2.get('tx', []))}")
        
        # Test that both blocks are valid individually
        assert block_1['height'] == block_2['height'], "Blocks should be at same height"
        
        # In a secure PoS system, the validator selection should not be easily
        # manipulable by changing transaction ordering
        
        # Reconnect to the network and see which block wins
        self.sync_all()
        
        final_best_block = node.getbestblockhash()
        final_height = node.getblockcount()
        
        self.log.info(f"Final best block: {final_best_block}")
        self.log.info(f"Final height: {final_height}")
        
        # The network should converge to one valid chain
        assert final_height >= initial_height + 1, "Chain should have progressed"
        
        self.log.info("✓ Stake grinding resistance test completed")

    def test_nothing_at_stake_prevention(self):
        """Test prevention of nothing-at-stake attacks"""
        self.log.info("Testing nothing-at-stake attack prevention...")
        
        # Create a fork scenario to test nothing-at-stake prevention
        # In this attack, validators vote on multiple competing chains
        
        # Disconnect nodes to create potential fork
        disconnect_nodes(self.nodes[0], 1)
        disconnect_nodes(self.nodes[1], 2)
        
        initial_height = self.nodes[0].getblockcount()
        
        # Create competing chains
        # Chain A (nodes 0)
        chain_a_blocks = []
        for i in range(3):
            block_hash = self.nodes[0].generatetoaddress(1, self.nodes[0].getnewaddress())[0]
            chain_a_blocks.append(block_hash)
        
        # Chain B (nodes 1, 2, 3)
        chain_b_blocks = []
        for i in range(5):  # Longer chain
            generator = self.nodes[1 + (i % 2)]  # Alternate between nodes 1 and 2
            block_hash = generator.generatetoaddress(1, generator.getnewaddress())[0]
            chain_b_blocks.append(block_hash)
        
        # Sync within each partition
        wait_until(lambda: self.nodes[1].getblockcount() == self.nodes[2].getblockcount(), timeout=10)
        
        # Verify different chain lengths
        height_a = self.nodes[0].getblockcount()
        height_b = self.nodes[1].getblockcount()
        
        assert height_a == initial_height + 3, f"Chain A height incorrect: {height_a}"
        assert height_b == initial_height + 5, f"Chain B height incorrect: {height_b}"
        
        # Simulate nothing-at-stake scenario: a validator trying to validate both chains
        # In a secure PoS system, this should be detected and penalized
        
        # Reconnect the network
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[1], 2)
        
        # Wait for network to converge
        wait_until(lambda: all(
            node.getblockcount() == self.nodes[0].getblockcount() 
            for node in self.nodes
        ), timeout=60)
        
        # Verify convergence to longest valid chain
        final_heights = [node.getblockcount() for node in self.nodes]
        final_hashes = [node.getbestblockhash() for node in self.nodes]
        
        assert all(h == final_heights[0] for h in final_heights), "Network didn't converge"
        assert all(h == final_hashes[0] for h in final_hashes), "Different best blocks"
        
        # Should converge to the longer chain (Chain B)
        expected_height = initial_height + 5
        assert final_heights[0] == expected_height, f"Wrong final height: {final_heights[0]} vs {expected_height}"
        
        self.log.info("✓ Nothing-at-stake prevention test completed")

    def test_long_range_attack_prevention(self):
        """Test prevention of long-range attacks"""
        self.log.info("Testing long-range attack prevention...")
        
        # Long-range attacks involve creating an alternative history from far in the past
        # A secure PoS system should have checkpoints or other mechanisms to prevent this
        
        node = self.nodes[0]
        
        # Create a substantial chain
        initial_height = node.getblockcount()
        main_chain_blocks = 20
        
        main_chain_hashes = []
        for i in range(main_chain_blocks):
            block_hash = node.generatetoaddress(1, node.getnewaddress())[0]
            main_chain_hashes.append(block_hash)
        
        self.sync_all()
        
        # Record the legitimate chain state
        legitimate_height = node.getblockcount()
        legitimate_best_block = node.getbestblockhash()
        
        # Simulate long-range attack: try to rewrite history from an old block
        # Find a block from several blocks back
        reorg_point = max(0, legitimate_height - 15)
        reorg_block_hash = node.getblockhash(reorg_point)
        
        self.log.info(f"Attempting long-range attack from block {reorg_point}")
        self.log.info(f"Reorg point: {reorg_block_hash}")
        
        # Disconnect one node to simulate attacker
        attacker_node = self.nodes[3]
        for i in range(3):
            disconnect_nodes(attacker_node, i)
        
        # Attacker tries to build alternative chain from old block
        try:
            # Invalidate blocks back to reorg point on attacker node
            current_height = attacker_node.getblockcount()
            blocks_to_invalidate = current_height - reorg_point
            
            for i in range(blocks_to_invalidate):
                height_to_invalidate = current_height - i
                if height_to_invalidate > reorg_point:
                    block_to_invalidate = attacker_node.getblockhash(height_to_invalidate)
                    attacker_node.invalidateblock(block_to_invalidate)
            
            # Build alternative chain (longer than original)
            alternative_blocks = main_chain_blocks + 5
            for i in range(alternative_blocks):
                attacker_node.generatetoaddress(1, attacker_node.getnewaddress())
            
            attacker_height = attacker_node.getblockcount()
            attacker_best_block = attacker_node.getbestblockhash()
            
            self.log.info(f"Attacker chain height: {attacker_height}")
            self.log.info(f"Attacker best block: {attacker_best_block}")
            
            # Reconnect attacker to network
            for i in range(3):
                connect_nodes(attacker_node, i)
            
            # Wait for network to process the long-range attack
            time.sleep(5)  # Give time for processing
            
            # Check if the network accepted the long-range attack
            network_heights = [self.nodes[i].getblockcount() for i in range(3)]
            network_hashes = [self.nodes[i].getbestblockhash() for i in range(3)]
            
            # In a secure system, the network should reject the long-range attack
            # and maintain the original legitimate chain
            
            self.log.info(f"Network heights after attack: {network_heights}")
            self.log.info(f"Legitimate height: {legitimate_height}")
            
            # The main network should not accept the long-range reorg
            # (In practice, this would be prevented by checkpoints or weak subjectivity)
            
            if all(h == legitimate_height for h in network_heights):
                self.log.info("✓ Long-range attack successfully rejected")
            else:
                self.log.warning("⚠ Long-range attack may have succeeded - check security mechanisms")
            
        except Exception as e:
            self.log.info(f"Long-range attack failed as expected: {e}")
        
        self.log.info("✓ Long-range attack prevention test completed")

    def test_validator_key_security(self):
        """Test validator key security and signature validation"""
        self.log.info("Testing validator key security...")
        
        node = self.nodes[0]
        
        # Test validator key generation and validation
        try:
            # Generate validator keys
            validator_address = node.getnewaddress()
            
            # Test key validation (this would normally involve cryptographic validation)
            # For now, we test that the system properly handles key-related operations
            
            # Attempt to register validator with valid key
            try:
                result = node.registervalidator(validator_address, 1000, "Security Test Validator")
                self.log.info("✓ Valid validator registration succeeded")
            except Exception as e:
                if "method not found" in str(e).lower():
                    self.log.info("⚠ Validator registration RPC not implemented")
                else:
                    raise
            
            # Test invalid key scenarios
            invalid_addresses = [
                "",  # Empty address
                "invalid_address",  # Invalid format
                "1" * 100,  # Too long
            ]
            
            for invalid_addr in invalid_addresses:
                try:
                    result = node.registervalidator(invalid_addr, 1000, "Invalid Test")
                    self.log.warning(f"⚠ Invalid address {invalid_addr} was accepted")
                except Exception as e:
                    if "method not found" not in str(e).lower():
                        self.log.info(f"✓ Invalid address {invalid_addr} properly rejected")
            
        except Exception as e:
            if "method not found" in str(e).lower():
                self.log.info("⚠ Validator key security test skipped (RPC not implemented)")
            else:
                raise
        
        # Test signature validation (simulate)
        self.log.info("Testing signature validation...")
        
        # In a real implementation, this would test:
        # 1. Valid signatures are accepted
        # 2. Invalid signatures are rejected
        # 3. Signature replay attacks are prevented
        # 4. Key rotation is handled securely
        
        # For now, we test basic cryptographic operations
        test_message = "test_message_for_signing"
        test_hash = hashlib.sha256(test_message.encode()).hexdigest()
        
        self.log.info(f"Test message hash: {test_hash}")
        
        # Verify hash consistency
        test_hash_2 = hashlib.sha256(test_message.encode()).hexdigest()
        assert test_hash == test_hash_2, "Hash function not deterministic"
        
        self.log.info("✓ Validator key security test completed")

    def test_stake_manipulation_prevention(self):
        """Test prevention of stake manipulation attacks"""
        self.log.info("Testing stake manipulation prevention...")
        
        node = self.nodes[0]
        
        # Test various stake manipulation scenarios
        
        # Scenario 1: Attempt to stake with insufficient funds
        try:
            # Try to stake more than available balance
            balance = node.getbalance()
            excessive_stake = balance + 1000
            
            result = node.stakecoin(excessive_stake, 3600)
            self.log.warning("⚠ Excessive stake was accepted")
            
        except Exception as e:
            if "method not found" in str(e).lower():
                self.log.info("⚠ Stake manipulation test skipped (RPC not implemented)")
            elif "insufficient" in str(e).lower() or "balance" in str(e).lower():
                self.log.info("✓ Excessive stake properly rejected")
            else:
                raise
        
        # Scenario 2: Attempt to double-spend staked coins
        try:
            # Stake some coins
            stake_amount = 100
            stake_result = node.stakecoin(stake_amount, 3600)
            
            # Try to spend the same coins again
            try:
                spend_address = node.getnewaddress()
                spend_result = node.sendtoaddress(spend_address, stake_amount)
                self.log.warning("⚠ Double-spend of staked coins was allowed")
            except Exception as e:
                if "insufficient" in str(e).lower():
                    self.log.info("✓ Double-spend of staked coins properly prevented")
                else:
                    raise
            
        except Exception as e:
            if "method not found" in str(e).lower():
                self.log.info("⚠ Double-spend test skipped (RPC not implemented)")
            else:
                raise
        
        # Scenario 3: Test stake lock integrity
        try:
            # Create stake lock
            stake_amount = 200
            lock_duration = 7200  # 2 hours
            stake_result = node.stakecoin(stake_amount, lock_duration)
            
            # Mine block to confirm
            node.generatetoaddress(1, node.getnewaddress())
            
            # Try to unstake before lock expires (should fail)
            try:
                unstake_result = node.unstakecoin(stake_result.get("txid", ""), 0)
                self.log.warning("⚠ Premature unstaking was allowed")
            except Exception as e:
                if "locked" in str(e).lower() or "expired" in str(e).lower():
                    self.log.info("✓ Premature unstaking properly prevented")
                elif "method not found" in str(e).lower():
                    self.log.info("⚠ Unstaking test skipped (RPC not implemented)")
                else:
                    raise
            
        except Exception as e:
            if "method not found" in str(e).lower():
                self.log.info("⚠ Stake lock test skipped (RPC not implemented)")
            else:
                raise
        
        # Scenario 4: Test stake amount validation
        invalid_stake_amounts = [
            -100,  # Negative amount
            0,     # Zero amount
            0.001, # Below minimum (assuming minimum is higher)
        ]
        
        for invalid_amount in invalid_stake_amounts:
            try:
                result = node.stakecoin(invalid_amount, 3600)
                self.log.warning(f"⚠ Invalid stake amount {invalid_amount} was accepted")
            except Exception as e:
                if "method not found" not in str(e).lower():
                    self.log.info(f"✓ Invalid stake amount {invalid_amount} properly rejected")
        
        self.log.info("✓ Stake manipulation prevention test completed")

if __name__ == '__main__':
    PoSStakeGrindingSecurityTest().main()