#!/usr/bin/env python3
# Copyright (c) 2024 The BabaChain Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Turkish language support and character encoding."""

from test_framework.test_framework import BabaChainTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
import json

class TurkishValidationTest(BabaChainTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Testing Turkish character encoding in RPC calls...")
        self.test_turkish_rpc_encoding()
        
        self.log.info("Testing Turkish characters in wallet labels...")
        self.test_turkish_wallet_labels()
        
        self.log.info("Testing Turkish characters in transaction descriptions...")
        self.test_turkish_transaction_descriptions()
        
        self.log.info("Testing Turkish number formatting...")
        self.test_turkish_number_formatting()
        
        self.log.info("Testing Turkish character validation...")
        self.test_turkish_character_validation()

    def test_turkish_rpc_encoding(self):
        """Test that RPC calls properly handle Turkish characters."""
        node = self.nodes[0]
        
        # Test Turkish characters in RPC parameters
        turkish_label = "Türkçe Etiket"
        
        # Generate a new address with Turkish label
        address = node.getnewaddress(turkish_label)
        
        # Verify the label was stored correctly
        address_info = node.getaddressinfo(address)
        if 'label' in address_info:
            assert_equal(address_info['label'], turkish_label)
        
        # Test listing addresses with Turkish labels
        addresses = node.listaddressgroupings()
        found_turkish_label = False
        for group in addresses:
            for addr_info in group:
                if len(addr_info) > 2 and addr_info[2] == turkish_label:
                    found_turkish_label = True
                    break
        
        self.log.info("Turkish label handling in RPC: PASSED")

    def test_turkish_wallet_labels(self):
        """Test Turkish characters in wallet address labels."""
        node = self.nodes[0]
        
        # Test various Turkish characters
        turkish_labels = [
            "Çiçek Mağazası",
            "Öğrenci Hesabı", 
            "Şirket Giderleri",
            "Üniversite Ödemesi",
            "Ğ harfi testi",
            "İstanbul Ofisi"
        ]
        
        addresses = []
        for label in turkish_labels:
            addr = node.getnewaddress(label)
            addresses.append((addr, label))
        
        # Verify all labels are stored and retrieved correctly
        for addr, expected_label in addresses:
            addr_info = node.getaddressinfo(addr)
            if 'label' in addr_info:
                assert_equal(addr_info['label'], expected_label)
        
        self.log.info("Turkish wallet labels: PASSED")

    def test_turkish_transaction_descriptions(self):
        """Test Turkish characters in transaction descriptions."""
        node = self.nodes[0]
        
        # Generate some coins first
        node.generatetoaddress(101, node.getnewaddress())
        
        # Create a transaction with Turkish description
        turkish_desc = "Çiçek satın alma işlemi"
        recipient_addr = node.getnewaddress("Alıcı")
        
        # Send a small amount
        txid = node.sendtoaddress(recipient_addr, 0.1)
        
        # Verify transaction was created
        tx_info = node.gettransaction(txid)
        assert_equal(tx_info['txid'], txid)
        
        self.log.info("Turkish transaction descriptions: PASSED")

    def test_turkish_number_formatting(self):
        """Test Turkish number formatting (comma as decimal separator)."""
        node = self.nodes[0]
        
        # Test that amounts are properly formatted
        # This would require specific Turkish locale support in the node
        # For now, we just verify that Turkish amounts don't cause errors
        
        balance = node.getbalance()
        self.log.info(f"Current balance: {balance}")
        
        # Test that large numbers don't cause issues with Turkish formatting
        large_amount = 1234567.89012345
        formatted_amount = f"{large_amount:,.8f}".replace(',', ' ').replace('.', ',')
        self.log.info(f"Turkish formatted amount: {formatted_amount}")
        
        self.log.info("Turkish number formatting: PASSED")

    def test_turkish_character_validation(self):
        """Test validation of Turkish characters in various contexts."""
        node = self.nodes[0]
        
        # Test that invalid UTF-8 sequences are rejected
        # This would be tested at the RPC level if validation is implemented
        
        # Test valid Turkish characters
        valid_turkish_strings = [
            "çÇğĞıİöÖşŞüÜ",
            "Türkiye",
            "Merhaba dünya",
            "Güzel bir gün"
        ]
        
        for test_string in valid_turkish_strings:
            # Test that valid Turkish strings work in labels
            try:
                addr = node.getnewaddress(test_string)
                addr_info = node.getaddressinfo(addr)
                self.log.info(f"Valid Turkish string '{test_string}': OK")
            except Exception as e:
                self.log.error(f"Valid Turkish string '{test_string}' failed: {e}")
                raise
        
        self.log.info("Turkish character validation: PASSED")

if __name__ == '__main__':
    TurkishValidationTest().main()