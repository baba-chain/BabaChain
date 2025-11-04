#!/usr/bin/env python3
# Copyright (c) 2025 The BabaChain Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test complete blockchain synchronization with PoS consensus."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
    connect_nodes,
    disconnect_nodes,
    wait_until,
)
from decimal import Decimal
import time

class PoSBlockchainSyncTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 5
        self.extra_args = [
            ["-debug=pos", "-debug=net", "-debug=validation"],
            ["-debug=pos", "-debug=net", "-debug=validation"],
            ["-debug=pos", "-debug=net", "-debug=validation"],
            ["-debug=pos", "-debug=net", "-debug=validation"],
            ["-debug=pos", "-debug=net", "-debug=validation"],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()
        # Initially connect nodes in a line: 0-1-2-3-4
        for i in range(self.num_nodes - 1):
            connect_nodes(self.nodes[i], i + 1)
        self.sync_all()

    def run_test(self):
        self.log.info("Starting PoS blockchain synchronization tests...")
        
        # Test various synchronization scenarios
        self.test_initial_sync()
        self.test_fork_resolution()
        self.test_network_partition_recovery()
        self.test_new_node_sync()
        self.test_large_reorg_handling()
        
        self.log.info("PoS blockchain synchronization tests completed successfully")

    def test_initial_sync(self):
        """Test initial blockchain synchronization"""
        self.log.info("Testing initial blockchain synchronization...")
        
        # Generate initial blockchain on node 0
        node0 = self.nodes[0]
        node0.generatetoaddress(50, node0.getnewaddress())
        
        # Wait for all nodes to sync
        self.sync_all()
        
        # Verify all nodes have the same blockchain
        heights = [node.getblockcount() for node in self.nodes]
        hashes = [node.getbestblockhash() for node in self.nodes]
        
        assert all(h == heights[0] for h in heights), f"Heights not equal: {heights}"
        assert all(h == hashes[0] for h in hashes), f"Best block hashes not equal: {hashes}"
        
        # Verify genesis block is correct
        genesis_hash = node0.getblockhash(0)
        for i, node in enumerate(self.nodes):
            node_genesis = node.getblockhash(0)
            assert_equal(node_genesis, genesis_hash, f"Node {i} has different genesis block")
        
        self.log.info("✓ Initial synchronization working correctly")

    def test_fork_resolution(self):
        """Test fork resolution with PoS consensus"""
        self.log.info("Testing fork resolution...")
        
        # Create a temporary fork by disconnecting nodes
        disconnect_nodes(self.nodes[0], 1)
        disconnect_nodes(self.nodes[1], 2)
        
        # Generate blocks on different sides of the split
        # Side A: nodes 0
        # Side B: nodes 1, 2, 3, 4
        
        initial_height = self.nodes[0].getblockcount()
        
        # Generate blocks on side A (shorter chain)
        for i in range(3):
            self.nodes[0].generatetoaddress(1, self.nodes[0].getnewaddress())
        
        # Generate blocks on side B (longer chain)
        for i in range(5):
            self.nodes[2].generatetoaddress(1, self.nodes[2].getnewaddress())
        
        # Sync side B
        connect_nodes(self.nodes[1], 2)
        wait_until(lambda: self.nodes[1].getblockcount() == self.nodes[2].getblockcount(), timeout=10)
        
        # Verify different heights
        height_a = self.nodes[0].getblockcount()
        height_b = self.nodes[1].getblockcount()
        
        assert height_a == initial_height + 3, f"Side A height incorrect: {height_a}"
        assert height_b == initial_height + 5, f"Side B height incorrect: {height_b}"
        
        # Reconnect and resolve fork
        connect_nodes(self.nodes[0], 1)
        
        # Wait for fork resolution
        wait_until(lambda: self.nodes[0].getblockcount() == self.nodes[1].getblockcount(), timeout=30)
        
        # Verify all nodes converged to the longer chain
        final_heights = [node.getblockcount() for node in self.nodes]
        final_hashes = [node.getbestblockhash() for node in self.nodes]
        
        assert all(h == final_heights[0] for h in final_heights), "Fork resolution failed - heights differ"
        assert all(h == final_hashes[0] for h in final_hashes), "Fork resolution failed - best blocks differ"
        assert final_heights[0] == initial_height + 5, "Wrong chain selected in fork resolution"
        
        self.log.info("✓ Fork resolution working correctly")

    def test_network_partition_recovery(self):
        """Test recovery from network partition"""
        self.log.info("Testing network partition recovery...")
        
        # Create network partition: {0,1} vs {2,3,4}
        disconnect_nodes(self.nodes[1], 2)
        
        initial_height = self.nodes[0].getblockcount()
        
        # Generate blocks on both partitions
        partition_a_blocks = 8
        partition_b_blocks = 12
        
        # Partition A: nodes 0, 1
        for i in range(partition_a_blocks):
            generator = self.nodes[i % 2]
            generator.generatetoaddress(1, generator.getnewaddress())
            # Sync within partition
            wait_until(lambda: self.nodes[0].getblockcount() == self.nodes[1].getblockcount(), timeout=5)
        
        # Partition B: nodes 2, 3, 4
        for i in range(partition_b_blocks):
            generator = self.nodes[2 + (i % 3)]
            generator.generatetoaddress(1, generator.getnewaddress())
            # Sync within partition
            if i % 3 == 0:  # Sync every few blocks
                wait_until(lambda: (
                    self.nodes[2].getblockcount() == self.nodes[3].getblockcount() == self.nodes[4].getblockcount()
                ), timeout=5)
        
        # Verify partitions have different chains
        height_a = self.nodes[0].getblockcount()
        height_b = self.nodes[2].getblockcount()
        
        assert height_a == initial_height + partition_a_blocks
        assert height_b == initial_height + partition_b_blocks
        assert height_a != height_b
        
        # Heal the partition
        connect_nodes(self.nodes[1], 2)
        
        # Wait for network to converge
        wait_until(lambda: all(
            node.getblockcount() == self.nodes[0].getblockcount() 
            for node in self.nodes
        ), timeout=60)
        
        # Verify convergence to longest chain
        final_heights = [node.getblockcount() for node in self.nodes]
        final_hashes = [node.getbestblockhash() for node in self.nodes]
        
        assert all(h == final_heights[0] for h in final_heights), "Partition recovery failed"
        assert all(h == final_hashes[0] for h in final_hashes), "Partition recovery failed"
        
        # Should converge to the longer chain (partition B)
        expected_height = initial_height + max(partition_a_blocks, partition_b_blocks)
        assert final_heights[0] == expected_height, f"Wrong final height: {final_heights[0]} vs {expected_height}"
        
        self.log.info("✓ Network partition recovery working correctly")

    def test_new_node_sync(self):
        """Test new node joining and syncing from genesis"""
        self.log.info("Testing new node synchronization...")
        
        # Generate a substantial blockchain
        current_height = self.nodes[0].getblockcount()
        blocks_to_add = 25
        
        for i in range(blocks_to_add):
            generator = self.nodes[i % (self.num_nodes - 1)]  # Don't use last node yet
            generator.generatetoaddress(1, generator.getnewaddress())
            if i % 5 == 0:
                self.sync_all(self.nodes[:-1])  # Sync all except last node
        
        self.sync_all(self.nodes[:-1])
        
        # Verify blockchain grew
        new_height = self.nodes[0].getblockcount()
        assert new_height == current_height + blocks_to_add
        
        # Now connect the last node (simulating a new node joining)
        new_node = self.nodes[-1]
        initial_new_node_height = new_node.getblockcount()
        
        # Connect new node to the network
        connect_nodes(new_node, 0)
        connect_nodes(new_node, 1)
        
        # Wait for new node to sync
        target_height = self.nodes[0].getblockcount()
        wait_until(lambda: new_node.getblockcount() == target_height, timeout=120)
        
        # Verify new node has correct blockchain
        assert_equal(new_node.getblockcount(), target_height)
        assert_equal(new_node.getbestblockhash(), self.nodes[0].getbestblockhash())
        
        # Verify new node can participate in consensus
        new_node.generatetoaddress(1, new_node.getnewaddress())
        self.sync_all()
        
        # All nodes should have the same new height
        final_heights = [node.getblockcount() for node in self.nodes]
        assert all(h == final_heights[0] for h in final_heights)
        
        self.log.info("✓ New node synchronization working correctly")

    def test_large_reorg_handling(self):
        """Test handling of large reorganizations"""
        self.log.info("Testing large reorganization handling...")
        
        # Create a scenario where a large reorg is needed
        # Disconnect majority of network
        for i in range(1, self.num_nodes):
            disconnect_nodes(self.nodes[0], i)
        
        initial_height = self.nodes[0].getblockcount()
        
        # Generate a long chain on isolated node 0
        minority_blocks = 15
        for i in range(minority_blocks):
            self.nodes[0].generatetoaddress(1, self.nodes[0].getnewaddress())
        
        # Generate an even longer chain on the majority network
        majority_blocks = 20
        for i in range(majority_blocks):
            generator = self.nodes[1 + (i % (self.num_nodes - 1))]
            generator.generatetoaddress(1, generator.getnewaddress())
            if i % 5 == 0:
                # Sync majority network
                for j in range(1, self.num_nodes - 1):
                    wait_until(lambda: (
                        self.nodes[j].getblockcount() == self.nodes[j + 1].getblockcount()
                    ), timeout=10)
        
        # Verify different chain lengths
        minority_height = self.nodes[0].getblockcount()
        majority_height = self.nodes[1].getblockcount()
        
        assert minority_height == initial_height + minority_blocks
        assert majority_height == initial_height + majority_blocks
        
        # Reconnect node 0 to trigger large reorg
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[0], 2)
        
        # Wait for reorg to complete
        target_height = max(minority_height, majority_height)
        wait_until(lambda: self.nodes[0].getblockcount() == target_height, timeout=180)
        
        # Verify all nodes converged
        self.sync_all()
        
        final_heights = [node.getblockcount() for node in self.nodes]
        final_hashes = [node.getbestblockhash() for node in self.nodes]
        
        assert all(h == final_heights[0] for h in final_heights), "Large reorg failed"
        assert all(h == final_hashes[0] for h in final_hashes), "Large reorg failed"
        assert final_heights[0] == initial_height + majority_blocks, "Wrong chain after reorg"
        
        self.log.info("✓ Large reorganization handling working correctly")

if __name__ == '__main__':
    PoSBlockchainSyncTest().main()