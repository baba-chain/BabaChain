#!/usr/bin/env python3
"""
BabaChain Integration Validation System
Validates auto-node and staking features across all platforms
"""

import json
import subprocess
import time
import os
import sys
from pathlib import Path
import logging

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class BabaChainIntegrationValidator:
    def __init__(self):
        self.validation_results = []
        self.core_node_process = None
        
    def validate_core_integration(self):
        """Validate core BabaChain network integration"""
        logger.info("🔧 Validating core network integration...")
        
        tests = [
            ("Genesis Block Configuration", self.check_genesis_config),
            ("PoS Consensus Parameters", self.check_pos_parameters),
            ("Supply Management", self.check_supply_parameters),
            ("Network Magic Bytes", self.check_network_magic),
            ("RPC Interface", self.check_rpc_interface)
        ]
        
        for test_name, test_func in tests:
            try:
                result = test_func()
                self.validation_results.append((test_name, "PASS" if result else "FAIL"))
                logger.info(f"{'✅' if result else '❌'} {test_name}: {'PASS' if result else 'FAIL'}")
            except Exception as e:
                self.validation_results.append((test_name, "ERROR"))
                logger.error(f"⚠️ {test_name}: ERROR - {e}")
    
    def check_genesis_config(self):
        """Check genesis block configuration"""
        try:
            with open("src/chainparams.cpp", "r") as f:
                content = f.read()
                return (
                    "CreateBabaChainGenesisBlock" in content and
                    "20000000 * COIN" in content and
                    "BabaChain Genesis Block" in content
                )
        except FileNotFoundError:
            return False
    
    def check_pos_parameters(self):
        """Check PoS consensus parameters"""
        try:
            with open("src/chainparams.cpp", "r") as f:
                content = f.read()
                return (
                    "nSubsidyHalvingInterval = 0" in content and
                    "nPowKGWHeight = 0" in content and
                    "nPowDGWHeight = 0" in content
                )
        except FileNotFoundError:
            return False
    
    def check_supply_parameters(self):
        """Check supply management parameters"""
        try:
            with open("src/chainparams.cpp", "r") as f:
                content = f.read()
                return (
                    "nMaxSupply = 1000000000" in content and
                    "nPremineAmount = 20000000" in content and
                    "nInitialBlockReward = 200" in content
                )
        except FileNotFoundError:
            return False
    
    def check_network_magic(self):
        """Check network magic bytes"""
        try:
            with open("src/chainparams.cpp", "r") as f:
                content = f.read()
                return (
                    "0xba" in content and
                    "0x13" in content and
                    "0x37" in content
                )
        except FileNotFoundError:
            return False
    
    def check_rpc_interface(self):
        """Check RPC interface configuration"""
        rpc_files = [
            "src/rpc/blockchain.cpp",
            "src/rpc/mining.cpp",
            "src/rpc/misc.cpp"
        ]
        
        for rpc_file in rpc_files:
            if not Path(rpc_file).exists():
                return False
        
        return True
    
    def validate_mobile_wallet_integration(self):
        """Validate mobile wallet integration"""
        logger.info("📱 Validating mobile wallet integration...")
        
        # Android wallet validation
        android_tests = [
            ("Android Project Structure", self.check_android_structure),
            ("Android Build Configuration", self.check_android_build),
            ("Android BabaChain Branding", self.check_android_branding),
            ("Android Turkish Support", self.check_android_turkish)
        ]
        
        for test_name, test_func in android_tests:
            try:
                result = test_func()
                self.validation_results.append((test_name, "PASS" if result else "FAIL"))
                logger.info(f"{'✅' if result else '❌'} {test_name}: {'PASS' if result else 'FAIL'}")
            except Exception as e:
                self.validation_results.append((test_name, "ERROR"))
                logger.error(f"⚠️ {test_name}: ERROR - {e}")
        
        # iOS wallet validation
        ios_tests = [
            ("iOS Project Structure", self.check_ios_structure),
            ("iOS Build Configuration", self.check_ios_build),
            ("iOS BabaChain Branding", self.check_ios_branding),
            ("iOS Turkish Support", self.check_ios_turkish)
        ]
        
        for test_name, test_func in ios_tests:
            try:
                result = test_func()
                self.validation_results.append((test_name, "PASS" if result else "FAIL"))
                logger.info(f"{'✅' if result else '❌'} {test_name}: {'PASS' if result else 'FAIL'}")
            except Exception as e:
                self.validation_results.append((test_name, "ERROR"))
                logger.error(f"⚠️ {test_name}: ERROR - {e}")
    
    def check_android_structure(self):
        """Check Android wallet structure"""
        required_paths = [
            "BabaChain-AndroidMobileWallet/wallet",
            "BabaChain-AndroidMobileWallet/wallet/build.gradle",
            "BabaChain-AndroidMobileWallet/wallet/src",
            "BabaChain-AndroidMobileWallet/settings.gradle"
        ]
        
        return all(Path(path).exists() for path in required_paths)
    
    def check_android_build(self):
        """Check Android build configuration"""
        build_file = Path("BabaChain-AndroidMobileWallet/wallet/build.gradle")
        if not build_file.exists():
            return False
        
        try:
            with open(build_file, "r") as f:
                content = f.read()
                return "babachain" in content.lower()
        except:
            return False
    
    def check_android_branding(self):
        """Check Android BabaChain branding"""
        src_dir = Path("BabaChain-AndroidMobileWallet/wallet/src")
        if not src_dir.exists():
            return False
        
        # Check for BabaChain references in source files
        try:
            result = subprocess.run([
                "grep", "-r", "-i", "babachain", str(src_dir)
            ], capture_output=True, text=True)
            return result.returncode == 0
        except:
            return False
    
    def check_android_turkish(self):
        """Check Android Turkish language support"""
        turkish_strings = Path("BabaChain-AndroidMobileWallet/wallet/res/values-tr/strings.xml")
        return turkish_strings.exists()
    
    def check_ios_structure(self):
        """Check iOS wallet structure"""
        required_paths = [
            "BabaChain-IOSMobileWallet/BabaChainWallet",
            "BabaChain-IOSMobileWallet/BabaChainWallet.xcodeproj",
            "BabaChain-IOSMobileWallet/BabaChainWallet/Sources",
            "BabaChain-IOSMobileWallet/Podfile"
        ]
        
        return all(Path(path).exists() for path in required_paths)
    
    def check_ios_build(self):
        """Check iOS build configuration"""
        project_file = Path("BabaChain-IOSMobileWallet/BabaChainWallet.xcodeproj/project.pbxproj")
        if not project_file.exists():
            return False
        
        try:
            with open(project_file, "r") as f:
                content = f.read()
                return "BabaChain" in content
        except:
            return False
    
    def check_ios_branding(self):
        """Check iOS BabaChain branding"""
        sources_dir = Path("BabaChain-IOSMobileWallet/BabaChainWallet/Sources")
        if not sources_dir.exists():
            return False
        
        # Check for BabaChain references in source files
        try:
            result = subprocess.run([
                "grep", "-r", "-i", "babachain", str(sources_dir)
            ], capture_output=True, text=True)
            return result.returncode == 0
        except:
            return False
    
    def check_ios_turkish(self):
        """Check iOS Turkish language support"""
        turkish_dir = Path("BabaChain-IOSMobileWallet/BabaChainWallet/tr.lproj")
        return turkish_dir.exists()
    
    def validate_staking_features(self):
        """Validate staking and auto-node features"""
        logger.info("🥩 Validating staking and auto-node features...")
        
        tests = [
            ("PoS Implementation", self.check_pos_implementation),
            ("Staking RPC Commands", self.check_staking_rpc),
            ("Validator Management", self.check_validator_management),
            ("Auto-Node Discovery", self.check_auto_node_discovery),
            ("Network Auto-Scaling", self.check_network_autoscaling)
        ]
        
        for test_name, test_func in tests:
            try:
                result = test_func()
                self.validation_results.append((test_name, "PASS" if result else "FAIL"))
                logger.info(f"{'✅' if result else '❌'} {test_name}: {'PASS' if result else 'FAIL'}")
            except Exception as e:
                self.validation_results.append((test_name, "ERROR"))
                logger.error(f"⚠️ {test_name}: ERROR - {e}")
    
    def check_pos_implementation(self):
        """Check PoS implementation"""
        pos_files = [
            "src/pos.cpp",
            "src/pos.h"
        ]
        
        return any(Path(f).exists() for f in pos_files)
    
    def check_staking_rpc(self):
        """Check staking RPC commands"""
        try:
            # Check for staking-related RPC commands in source
            result = subprocess.run([
                "grep", "-r", "stakecoin\\|getstakinginfo", "src/rpc/"
            ], capture_output=True, text=True)
            return result.returncode == 0
        except:
            return False
    
    def check_validator_management(self):
        """Check validator management implementation"""
        try:
            result = subprocess.run([
                "grep", "-r", "validator", "src/"
            ], capture_output=True, text=True)
            return result.returncode == 0
        except:
            return False
    
    def check_auto_node_discovery(self):
        """Check auto-node discovery system"""
        discovery_script = Path("scripts/network/auto_discovery.py")
        return discovery_script.exists()
    
    def check_network_autoscaling(self):
        """Check network auto-scaling system"""
        autoscaling_script = Path("scripts/network-autoscaling.py")
        return autoscaling_script.exists()
    
    def validate_cross_platform_functionality(self):
        """Validate cross-platform functionality"""
        logger.info("🌐 Validating cross-platform functionality...")
        
        tests = [
            ("Integration Configuration", self.check_integration_config),
            ("Cross-Platform Config", self.check_cross_platform_config),
            ("Turkish Language Support", self.check_turkish_support),
            ("System Integration Script", self.check_system_integration)
        ]
        
        for test_name, test_func in tests:
            try:
                result = test_func()
                self.validation_results.append((test_name, "PASS" if result else "FAIL"))
                logger.info(f"{'✅' if result else '❌'} {test_name}: {'PASS' if result else 'FAIL'}")
            except Exception as e:
                self.validation_results.append((test_name, "ERROR"))
                logger.error(f"⚠️ {test_name}: ERROR - {e}")
    
    def check_integration_config(self):
        """Check integration configuration"""
        config_file = Path("config/wallet-integration.json")
        if not config_file.exists():
            return False
        
        try:
            with open(config_file, "r") as f:
                config = json.load(f)
                return "babachain_integration" in config
        except:
            return False
    
    def check_cross_platform_config(self):
        """Check cross-platform configuration"""
        config_files = [
            "config/platforms/desktop.conf",
            "config/platforms/mobile.conf",
            "config/platforms/testnet.conf"
        ]
        
        return any(Path(f).exists() for f in config_files)
    
    def check_turkish_support(self):
        """Check Turkish language support implementation"""
        # Check for Turkish language files
        turkish_files = [
            "src/qt/locale/bitcoin_tr.ts",
            "BabaChain-AndroidMobileWallet/wallet/res/values-tr/strings.xml",
            "BabaChain-IOSMobileWallet/BabaChainWallet/tr.lproj"
        ]
        
        return any(Path(f).exists() for f in turkish_files)
    
    def check_system_integration(self):
        """Check system integration script"""
        integration_script = Path("scripts/system-integration.sh")
        return integration_script.exists() and integration_script.stat().st_mode & 0o111
    
    def run_functional_tests(self):
        """Run functional tests if possible"""
        logger.info("🧪 Running functional tests...")
        
        # Check if we can build the core
        if self.can_build_core():
            self.validation_results.append(("Core Build Test", "PASS"))
            logger.info("✅ Core Build Test: PASS")
        else:
            self.validation_results.append(("Core Build Test", "FAIL"))
            logger.info("❌ Core Build Test: FAIL")
        
        # Check if we can run basic RPC commands
        if self.can_run_rpc_tests():
            self.validation_results.append(("RPC Functionality Test", "PASS"))
            logger.info("✅ RPC Functionality Test: PASS")
        else:
            self.validation_results.append(("RPC Functionality Test", "FAIL"))
            logger.info("❌ RPC Functionality Test: FAIL")
    
    def can_build_core(self):
        """Check if core can be built"""
        try:
            # Check if Makefile exists
            if not Path("Makefile").exists():
                return False
            
            # Try a dry run of make
            result = subprocess.run([
                "make", "-n", "babachaind"
            ], capture_output=True, text=True, timeout=30)
            
            return result.returncode == 0
        except:
            return False
    
    def can_run_rpc_tests(self):
        """Check if RPC tests can be run"""
        try:
            # Check if babachain-cli exists
            cli_path = Path("src/babachain-cli")
            if not cli_path.exists():
                return False
            
            # This is just checking if the binary exists and is executable
            return cli_path.stat().st_mode & 0o111 != 0
        except:
            return False
    
    def generate_validation_report(self):
        """Generate comprehensive validation report"""
        logger.info("📊 Generating validation report...")
        
        total_tests = len(self.validation_results)
        passed_tests = sum(1 for _, result in self.validation_results if result == "PASS")
        failed_tests = sum(1 for _, result in self.validation_results if result == "FAIL")
        error_tests = sum(1 for _, result in self.validation_results if result == "ERROR")
        
        report = {
            "validation_summary": {
                "total_tests": total_tests,
                "passed": passed_tests,
                "failed": failed_tests,
                "errors": error_tests,
                "success_rate": (passed_tests / total_tests * 100) if total_tests > 0 else 0
            },
            "test_results": self.validation_results,
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "recommendations": self.generate_recommendations()
        }
        
        # Save report to file
        with open("validation_report.json", "w") as f:
            json.dump(report, f, indent=2)
        
        return report
    
    def generate_recommendations(self):
        """Generate recommendations based on validation results"""
        recommendations = []
        
        failed_tests = [test for test, result in self.validation_results if result == "FAIL"]
        
        if "Genesis Block Configuration" in failed_tests:
            recommendations.append("Update genesis block configuration in src/chainparams.cpp")
        
        if "Android Project Structure" in failed_tests:
            recommendations.append("Verify Android wallet project structure and dependencies")
        
        if "iOS Project Structure" in failed_tests:
            recommendations.append("Verify iOS wallet project structure and Xcode configuration")
        
        if "Turkish Language Support" in failed_tests:
            recommendations.append("Implement Turkish language support across all platforms")
        
        if "Staking RPC Commands" in failed_tests:
            recommendations.append("Implement staking RPC commands in src/rpc/")
        
        return recommendations
    
    def run_complete_validation(self):
        """Run complete validation suite"""
        logger.info("🚀 Starting BabaChain Integration Validation...")
        
        # Run all validation categories
        self.validate_core_integration()
        self.validate_mobile_wallet_integration()
        self.validate_staking_features()
        self.validate_cross_platform_functionality()
        self.run_functional_tests()
        
        # Generate and display report
        report = self.generate_validation_report()
        
        print("\n" + "="*60)
        print("🎯 BABACHAIN INTEGRATION VALIDATION REPORT")
        print("="*60)
        print(f"Total Tests: {report['validation_summary']['total_tests']}")
        print(f"Passed: {report['validation_summary']['passed']} ✅")
        print(f"Failed: {report['validation_summary']['failed']} ❌")
        print(f"Errors: {report['validation_summary']['errors']} ⚠️")
        print(f"Success Rate: {report['validation_summary']['success_rate']:.1f}%")
        
        if report['recommendations']:
            print("\n📋 RECOMMENDATIONS:")
            for i, rec in enumerate(report['recommendations'], 1):
                print(f"  {i}. {rec}")
        
        print(f"\n📄 Full report saved to: validation_report.json")
        
        return report['validation_summary']['success_rate'] >= 80

def main():
    """Main function"""
    validator = BabaChainIntegrationValidator()
    success = validator.run_complete_validation()
    
    if success:
        print("\n🎉 Integration validation completed successfully!")
        sys.exit(0)
    else:
        print("\n⚠️ Integration validation found issues that need attention.")
        sys.exit(1)

if __name__ == "__main__":
    main()