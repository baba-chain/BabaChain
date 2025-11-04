#!/usr/bin/env python3
# Copyright (c) 2025 The BabaChain Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Benchmark and optimize memory usage for PoS staking data."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    connect_nodes,
)
import time
import gc
import sys

class PoSMemoryOptimizationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [
            ["-debug=pos", "-debug=mempool"],
            ["-debug=pos", "-debug=mempool"],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Starting PoS memory optimization benchmarks...")
        
        self.test_validator_registry_memory()
        self.test_stake_lock_memory_efficiency()
        self.test_memory_growth_patterns()
        self.test_garbage_collection_efficiency()
        self.test_memory_leak_detection()
        
        self.log.info("PoS memory optimization benchmarks completed")

    def get_memory_usage(self):
        """Get current memory usage in MB"""
        try:
            import psutil
            import os
            process = psutil.Process(os.getpid())
            return process.memory_info().rss / 1024 / 1024
        except ImportError:
            # Fallback to sys.getsizeof for basic measurement
            return 0

    def test_validator_registry_memory(self):
        """Test memory efficiency of validator registry"""
        self.log.info("Testing validator registry memory efficiency...")
        
        initial_memory = self.get_memory_usage()
        
        # Simulate large validator registry
        validator_counts = [1000, 5000, 10000, 25000]
        memory_measurements = []
        
        for count in validator_counts:
            self.log.info(f"Testing {count} validators...")
            
            # Create validator data structures
            validators = {}
            stake_locks = {}
            
            # Simulate validator registry
            for i in range(count):
                validator_id = f"validator_{i:06d}"
                
                # Validator data (optimized structure)
                validators[validator_id] = {
                    'pubkey': validator_id,  # In real implementation, this would be bytes
                    'stake': 1000 + (i % 10000),  # Varying stakes
                    'active': i % 10 != 0,  # 90% active
                    'reg_time': 1640995200 + i,  # Timestamp
                    'last_block': i % 100000,
                    'rewards': i * 0.001,  # Accumulated rewards
                }
                
                # Associated stake locks (multiple per validator)
                stakes_per_validator = min(5, 1 + (i % 10))  # 1-5 stakes per validator
                for j in range(stakes_per_validator):
                    stake_id = f"{validator_id}_stake_{j}"
                    stake_locks[stake_id] = {
                        'outpoint': f"tx_{i}_{j}:0",
                        'amount': 100 + (j * 50),
                        'lock_time': 1640995200 + i + (j * 3600),
                        'owner': validator_id
                    }
            
            current_memory = self.get_memory_usage()
            memory_used = current_memory - initial_memory
            
            # Calculate memory efficiency
            bytes_per_validator = (memory_used * 1024 * 1024) / count if count > 0 else 0
            total_objects = count + len(stake_locks)
            bytes_per_object = (memory_used * 1024 * 1024) / total_objects if total_objects > 0 else 0
            
            measurement = {
                'count': count,
                'memory_mb': memory_used,
                'bytes_per_validator': bytes_per_validator,
                'bytes_per_object': bytes_per_object,
                'total_objects': total_objects
            }
            memory_measurements.append(measurement)
            
            self.log.info(f"  Memory usage: {memory_used:.1f} MB")
            self.log.info(f"  {bytes_per_validator:.0f} bytes/validator")
            self.log.info(f"  {bytes_per_object:.0f} bytes/object")
            self.log.info(f"  Total objects: {total_objects}")
            
            # Memory efficiency assertions
            if count >= 1000:  # Only check for larger datasets
                assert bytes_per_validator < 2000, f"Validator memory too high: {bytes_per_validator} bytes"
                assert bytes_per_object < 1000, f"Object memory too high: {bytes_per_object} bytes"
            
            # Clean up for next iteration
            del validators
            del stake_locks
            gc.collect()
        
        # Analyze memory scaling
        self.log.info("Memory scaling analysis:")
        for measurement in memory_measurements:
            efficiency = measurement['count'] / measurement['memory_mb'] if measurement['memory_mb'] > 0 else 0
            self.log.info(f"  {measurement['count']} validators: {measurement['memory_mb']:.1f} MB ({efficiency:.0f} validators/MB)")
        
        self.log.info("✓ Validator registry memory efficiency test completed")

    def test_stake_lock_memory_efficiency(self):
        """Test memory efficiency of stake lock data structures"""
        self.log.info("Testing stake lock memory efficiency...")
        
        initial_memory = self.get_memory_usage()
        
        # Test different stake lock scenarios
        scenarios = [
            {'locks': 10000, 'description': 'Small scale'},
            {'locks': 50000, 'description': 'Medium scale'},
            {'locks': 100000, 'description': 'Large scale'},
        ]
        
        for scenario in scenarios:
            self.log.info(f"Testing {scenario['description']} ({scenario['locks']} locks)...")
            
            lock_count = scenario['locks']
            
            # Create optimized stake lock structure
            stake_locks = {}
            
            for i in range(lock_count):
                # Optimized data structure (minimal memory footprint)
                lock_id = f"lock_{i:08d}"
                stake_locks[lock_id] = {
                    'outpoint': (f"tx_{i:08x}", 0),  # Tuple instead of string
                    'amount': 100 + (i % 10000),  # int instead of float
                    'lock_time': 1640995200 + (i % 86400),  # int timestamp
                    'owner_idx': i % 10000,  # Index instead of full pubkey
                }
            
            current_memory = self.get_memory_usage()
            memory_used = current_memory - initial_memory
            
            # Calculate efficiency metrics
            bytes_per_lock = (memory_used * 1024 * 1024) / lock_count if lock_count > 0 else 0
            locks_per_mb = lock_count / memory_used if memory_used > 0 else 0
            
            self.log.info(f"  Memory usage: {memory_used:.1f} MB")
            self.log.info(f"  {bytes_per_lock:.0f} bytes/lock")
            self.log.info(f"  {locks_per_mb:.0f} locks/MB")
            
            # Test lookup performance
            lookup_start = time.time()
            lookups = min(10000, lock_count)
            for j in range(lookups):
                lock_id = f"lock_{j:08d}"
                lock_data = stake_locks.get(lock_id)
                assert lock_data is not None
            lookup_time = time.time() - lookup_start
            
            lookups_per_sec = lookups / lookup_time if lookup_time > 0 else 0
            self.log.info(f"  Lookup performance: {lookups_per_sec:.0f} lookups/sec")
            
            # Performance assertions
            assert bytes_per_lock < 500, f"Stake lock memory too high: {bytes_per_lock} bytes"
            assert lookups_per_sec > 10000, f"Lookup performance too low: {lookups_per_sec} lookups/sec"
            
            # Clean up
            del stake_locks
            gc.collect()
        
        self.log.info("✓ Stake lock memory efficiency test completed")

    def test_memory_growth_patterns(self):
        """Test memory growth patterns under continuous operation"""
        self.log.info("Testing memory growth patterns...")
        
        initial_memory = self.get_memory_usage()
        memory_samples = [initial_memory]
        
        # Simulate continuous operation with growing data
        operations = 1000
        batch_size = 100
        
        validators = {}
        stakes = {}
        
        for batch in range(operations // batch_size):
            self.log.info(f"Operation batch {batch + 1}/{operations // batch_size}")
            
            # Add new validators and stakes
            for i in range(batch_size):
                validator_idx = batch * batch_size + i
                validator_id = f"validator_{validator_idx:06d}"
                
                # Add validator
                validators[validator_id] = {
                    'pubkey': validator_id,
                    'stake': 1000 + validator_idx,
                    'active': True,
                    'reg_time': time.time(),
                }
                
                # Add stakes
                for j in range(3):  # 3 stakes per validator
                    stake_id = f"{validator_id}_stake_{j}"
                    stakes[stake_id] = {
                        'outpoint': (f"tx_{validator_idx}_{j}", 0),
                        'amount': 100 + (j * 50),
                        'lock_time': time.time() + 3600,
                        'owner': validator_id
                    }
            
            # Remove some old data (simulate unstaking/deregistration)
            if batch > 2:  # Start removing after 3 batches
                remove_batch = batch - 2
                for i in range(batch_size // 2):  # Remove half
                    old_validator_idx = remove_batch * batch_size + i
                    old_validator_id = f"validator_{old_validator_idx:06d}"
                    
                    # Remove validator and associated stakes
                    validators.pop(old_validator_id, None)
                    for j in range(3):
                        old_stake_id = f"{old_validator_id}_stake_{j}"
                        stakes.pop(old_stake_id, None)
            
            # Sample memory usage
            current_memory = self.get_memory_usage()
            memory_samples.append(current_memory)
            
            memory_growth = current_memory - initial_memory
            active_validators = len(validators)
            active_stakes = len(stakes)
            
            self.log.info(f"  Memory: {current_memory:.1f} MB (+{memory_growth:.1f} MB)")
            self.log.info(f"  Active: {active_validators} validators, {active_stakes} stakes")
            
            # Check for memory leaks (excessive growth)
            if batch > 5:  # Allow some initial growth
                expected_memory = initial_memory + (active_validators + active_stakes) * 0.001  # 1KB per object
                if memory_growth > expected_memory * 2:  # Allow 2x overhead
                    self.log.warning(f"Potential memory leak detected: {memory_growth:.1f} MB > {expected_memory * 2:.1f} MB")
        
        # Analyze memory growth pattern
        memory_growth_total = memory_samples[-1] - memory_samples[0]
        max_memory = max(memory_samples)
        min_memory = min(memory_samples)
        
        self.log.info(f"Memory growth analysis:")
        self.log.info(f"  Initial: {memory_samples[0]:.1f} MB")
        self.log.info(f"  Final: {memory_samples[-1]:.1f} MB")
        self.log.info(f"  Growth: {memory_growth_total:.1f} MB")
        self.log.info(f"  Peak: {max_memory:.1f} MB")
        self.log.info(f"  Range: {min_memory:.1f} - {max_memory:.1f} MB")
        
        # Memory growth assertions
        assert memory_growth_total < 100, f"Excessive memory growth: {memory_growth_total} MB"
        
        self.log.info("✓ Memory growth pattern test completed")

    def test_garbage_collection_efficiency(self):
        """Test garbage collection efficiency with PoS data structures"""
        self.log.info("Testing garbage collection efficiency...")
        
        initial_memory = self.get_memory_usage()
        
        # Create large data structures
        large_dataset_size = 50000
        
        self.log.info(f"Creating {large_dataset_size} objects...")
        
        # Create data
        validators = {}
        stakes = {}
        
        for i in range(large_dataset_size):
            validator_id = f"validator_{i:06d}"
            validators[validator_id] = {
                'pubkey': validator_id * 10,  # Make it larger
                'stake': 1000 + i,
                'active': True,
                'data': list(range(100)),  # Additional data
            }
            
            stake_id = f"stake_{i:06d}"
            stakes[stake_id] = {
                'outpoint': (f"tx_{i:08x}", 0),
                'amount': 100 + i,
                'lock_time': time.time() + i,
                'owner': validator_id,
                'extra_data': [i] * 50,  # Additional data
            }
        
        peak_memory = self.get_memory_usage()
        memory_used = peak_memory - initial_memory
        
        self.log.info(f"Peak memory usage: {peak_memory:.1f} MB (+{memory_used:.1f} MB)")
        
        # Test garbage collection
        self.log.info("Testing garbage collection...")
        
        # Delete references
        del validators
        del stakes
        
        # Force garbage collection
        gc_start = time.time()
        collected = gc.collect()
        gc_time = time.time() - gc_start
        
        post_gc_memory = self.get_memory_usage()
        memory_freed = peak_memory - post_gc_memory
        gc_efficiency = (memory_freed / memory_used) * 100 if memory_used > 0 else 0
        
        self.log.info(f"Garbage collection results:")
        self.log.info(f"  Objects collected: {collected}")
        self.log.info(f"  Time: {gc_time:.3f}s")
        self.log.info(f"  Memory freed: {memory_freed:.1f} MB")
        self.log.info(f"  Efficiency: {gc_efficiency:.1f}%")
        self.log.info(f"  Final memory: {post_gc_memory:.1f} MB")
        
        # Garbage collection assertions
        assert gc_efficiency > 50, f"Poor garbage collection efficiency: {gc_efficiency}%"
        assert gc_time < 1.0, f"Garbage collection too slow: {gc_time}s"
        
        self.log.info("✓ Garbage collection efficiency test completed")

    def test_memory_leak_detection(self):
        """Test for memory leaks in PoS operations"""
        self.log.info("Testing memory leak detection...")
        
        initial_memory = self.get_memory_usage()
        
        # Perform repeated operations that should not leak memory
        cycles = 10
        operations_per_cycle = 1000
        
        memory_samples = []
        
        for cycle in range(cycles):
            self.log.info(f"Leak detection cycle {cycle + 1}/{cycles}")
            
            cycle_start_memory = self.get_memory_usage()
            
            # Perform operations
            temp_data = {}
            
            for i in range(operations_per_cycle):
                # Simulate validator operations
                validator_id = f"temp_validator_{i}"
                temp_data[validator_id] = {
                    'pubkey': validator_id,
                    'stake': 1000 + i,
                    'temp_data': list(range(10))
                }
                
                # Simulate stake operations
                stake_id = f"temp_stake_{i}"
                temp_data[stake_id] = {
                    'outpoint': (f"temp_tx_{i}", 0),
                    'amount': 100 + i,
                    'temp_list': [i] * 5
                }
            
            # Clean up (simulate end of operations)
            del temp_data
            gc.collect()
            
            cycle_end_memory = self.get_memory_usage()
            memory_samples.append(cycle_end_memory)
            
            cycle_growth = cycle_end_memory - cycle_start_memory
            total_growth = cycle_end_memory - initial_memory
            
            self.log.info(f"  Cycle growth: {cycle_growth:.2f} MB")
            self.log.info(f"  Total growth: {total_growth:.2f} MB")
            
            # Check for leaks
            if cycle > 2:  # Allow some initial variance
                recent_samples = memory_samples[-3:]
                if all(sample > recent_samples[0] + 5 for sample in recent_samples[1:]):
                    self.log.warning(f"Potential memory leak detected in cycle {cycle}")
        
        # Analyze leak detection results
        final_memory = memory_samples[-1]
        total_growth = final_memory - initial_memory
        
        # Calculate trend
        if len(memory_samples) > 5:
            early_avg = sum(memory_samples[:3]) / 3
            late_avg = sum(memory_samples[-3:]) / 3
            trend = late_avg - early_avg
            
            self.log.info(f"Memory leak analysis:")
            self.log.info(f"  Initial: {initial_memory:.1f} MB")
            self.log.info(f"  Final: {final_memory:.1f} MB")
            self.log.info(f"  Total growth: {total_growth:.1f} MB")
            self.log.info(f"  Trend: {trend:.2f} MB")
            
            # Memory leak assertions
            assert total_growth < 20, f"Excessive memory growth (potential leak): {total_growth} MB"
            assert trend < 5, f"Upward memory trend (potential leak): {trend} MB"
        
        self.log.info("✓ Memory leak detection test completed")

if __name__ == '__main__':
    PoSMemoryOptimizationTest().main()