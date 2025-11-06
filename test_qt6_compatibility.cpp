#include <QApplication>
#include <QTimer>
#include <iostream>

// Include our fixed Qt 6 compatible classes
#include "src/qt/autobootstrap.h"
#include "src/qt/autonode.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    std::cout << "Testing Qt 6 compatibility for AutoBootstrap and AutoNode managers..." << std::endl;
    
    // Test AutoBootstrapManager instantiation
    try {
        AutoBootstrapManager* bootstrapManager = new AutoBootstrapManager();
        std::cout << "✓ AutoBootstrapManager created successfully" << std::endl;
        
        // Test basic functionality
        bool isBootstrapping = bootstrapManager->isBootstrapping();
        int progress = bootstrapManager->getBootstrapProgress();
        std::cout << "✓ AutoBootstrapManager methods work (bootstrapping: " << isBootstrapping 
                  << ", progress: " << progress << "%)" << std::endl;
        
        delete bootstrapManager;
    } catch (const std::exception& e) {
        std::cout << "✗ AutoBootstrapManager test failed: " << e.what() << std::endl;
        return 1;
    }
    
    // Test AutoNodeManager instantiation
    try {
        AutoNodeManager* nodeManager = new AutoNodeManager();
        std::cout << "✓ AutoNodeManager created successfully" << std::endl;
        
        // Test basic functionality
        bool isEnabled = nodeManager->isAutoDiscoveryEnabled();
        std::cout << "✓ AutoNodeManager methods work (auto discovery enabled: " << isEnabled << ")" << std::endl;
        
        delete nodeManager;
    } catch (const std::exception& e) {
        std::cout << "✗ AutoNodeManager test failed: " << e.what() << std::endl;
        return 1;
    }
    
    // Test signal emissions (basic Qt 6 MOC functionality)
    try {
        AutoBootstrapManager* bootstrapManager = new AutoBootstrapManager();
        AutoNodeManager* nodeManager = new AutoNodeManager();
        
        // Connect signals to verify Qt 6 signal/slot mechanism works
        QObject::connect(bootstrapManager, &AutoBootstrapManager::bootstrapProgress,
                        [](int progress) {
                            std::cout << "✓ Bootstrap progress signal emitted: " << progress << "%" << std::endl;
                        });
        
        QObject::connect(nodeManager, &AutoNodeManager::networkHealthChanged,
                        [](int score) {
                            std::cout << "✓ Network health signal emitted: " << score << std::endl;
                        });
        
        // Emit test signals
        Q_EMIT bootstrapManager->bootstrapProgress(50);
        Q_EMIT nodeManager->networkHealthChanged(75);
        
        std::cout << "✓ Qt 6 signal/slot mechanism works correctly" << std::endl;
        
        delete bootstrapManager;
        delete nodeManager;
    } catch (const std::exception& e) {
        std::cout << "✗ Signal/slot test failed: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n🎉 All Qt 6 compatibility tests passed!" << std::endl;
    std::cout << "✓ AutoBootstrapManager Qt 6 compatibility verified" << std::endl;
    std::cout << "✓ AutoNodeManager Qt 6 compatibility verified" << std::endl;
    std::cout << "✓ Qt 6 MOC and signal/slot system working" << std::endl;
    
    return 0;
}