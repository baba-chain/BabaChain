#!/usr/bin/env python3
# Copyright (c) 2025 The BabaChain Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Run all PoS integration tests."""

import subprocess
import sys
import os
import time
from pathlib import Path

# PoS integration test suite
POS_TESTS = [
    'feature_pos_consensus.py',
    'feature_pos_staking.py', 
    'feature_pos_blockchain_sync.py',
]

def run_test(test_name, test_dir):
    """Run a single test and return result."""
    test_path = test_dir / test_name
    
    print(f"\n{'='*60}")
    print(f"Running: {test_name}")
    print(f"{'='*60}")
    
    start_time = time.time()
    
    try:
        result = subprocess.run(
            [sys.executable, str(test_path)],
            cwd=test_dir.parent.parent,  # Run from repo root
            capture_output=True,
            text=True,
            timeout=300  # 5 minute timeout per test
        )
        
        duration = time.time() - start_time
        
        if result.returncode == 0:
            print(f"✓ PASSED ({duration:.1f}s)")
            return True, duration, ""
        else:
            print(f"✗ FAILED ({duration:.1f}s)")
            print("STDOUT:", result.stdout[-1000:])  # Last 1000 chars
            print("STDERR:", result.stderr[-1000:])
            return False, duration, result.stderr
            
    except subprocess.TimeoutExpired:
        print(f"✗ TIMEOUT (>300s)")
        return False, 300, "Test timed out"
    except Exception as e:
        print(f"✗ ERROR: {e}")
        return False, 0, str(e)

def main():
    """Run all PoS integration tests."""
    print("BabaChain PoS Integration Test Suite")
    print("=" * 50)
    
    # Find test directory
    script_dir = Path(__file__).parent
    test_dir = script_dir
    
    # Verify test files exist
    missing_tests = []
    for test in POS_TESTS:
        if not (test_dir / test).exists():
            missing_tests.append(test)
    
    if missing_tests:
        print(f"ERROR: Missing test files: {missing_tests}")
        return 1
    
    # Run tests
    results = []
    total_time = 0
    
    for test in POS_TESTS:
        passed, duration, error = run_test(test, test_dir)
        results.append((test, passed, duration, error))
        total_time += duration
        
        if not passed:
            print(f"\nTest {test} failed with error:")
            print(error)
    
    # Summary
    print(f"\n{'='*60}")
    print("TEST SUMMARY")
    print(f"{'='*60}")
    
    passed_count = sum(1 for _, passed, _, _ in results if passed)
    total_count = len(results)
    
    for test, passed, duration, error in results:
        status = "PASS" if passed else "FAIL"
        print(f"{test:<35} {status:>8} ({duration:>5.1f}s)")
    
    print(f"\nResults: {passed_count}/{total_count} tests passed")
    print(f"Total time: {total_time:.1f}s")
    
    if passed_count == total_count:
        print("\n🎉 All PoS integration tests PASSED!")
        return 0
    else:
        print(f"\n❌ {total_count - passed_count} test(s) FAILED")
        return 1

if __name__ == '__main__':
    sys.exit(main())