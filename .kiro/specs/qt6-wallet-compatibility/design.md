# Design Document

## Overview

This design addresses Qt 6 compatibility issues in the BabaChain desktop wallet, focusing on fixing compilation errors in AutoBootstrap and AutoNode managers. The main challenges involve Qt API changes, deprecated functions, and proper QObject inheritance patterns.

## Architecture

### Current Issues Analysis

1. **QObject Inheritance Problems**: AutoBootstrapManager and AutoNodeManager classes have incorrect QObject inheritance setup
2. **Signal/Slot Connection Issues**: Qt 6 requires proper context objects for signal-slot connections
3. **API Deprecations**: Several Qt 5 APIs have been removed or changed in Qt 6
4. **Variable Naming Conflicts**: Member variables conflict with signal names
5. **Missing ClientModel Methods**: Code references non-existent methods in ClientModel

### Design Approach

The solution follows a systematic approach to update each problematic component while maintaining existing functionality:

1. **Fix QObject Inheritance**: Ensure proper parent-child relationships
2. **Update Signal/Slot Connections**: Use Qt 6 compatible connection syntax
3. **Replace Deprecated APIs**: Update to modern Qt 6 equivalents
4. **Resolve Naming Conflicts**: Rename conflicting variables and methods
5. **Update ClientModel Integration**: Use available API methods

## Components and Interfaces

### AutoBootstrapManager Class

**Current Issues:**
- Variable name conflict: `bootstrapProgress` used as both signal name and member variable
- Incorrect QObject inheritance in constructor calls
- Deprecated `fs.h` string() method usage
- Missing ClientModel API methods

**Design Solution:**
```cpp
class AutoBootstrapManager : public QObject
{
    Q_OBJECT

private:
    int m_bootstrapProgress;  // Renamed to avoid conflict
    QNetworkAccessManager* m_networkManager;
    QTimer* m_statusTimer;
    ClientModel* m_clientModel;

public:
    explicit AutoBootstrapManager(QObject* parent = nullptr);
    int getBootstrapProgress() const { return m_bootstrapProgress; }

Q_SIGNALS:
    void bootstrapProgress(int percentage);  // Signal name unchanged for API compatibility
};
```

**Key Changes:**
1. Rename member variable to `m_bootstrapProgress`
2. Proper QObject parent passing in constructor
3. Use `fs::path::string()` alternative or `QString::fromStdString()`
4. Replace `clientModel->getVerificationProgress()` with available signal data

### AutoNodeManager Class

**Current Issues:**
- `std::random_shuffle` deprecated in C++17, removed in C++20
- QNetworkRequest initialization syntax issues
- Missing ClientModel API methods
- Unused variables causing warnings

**Design Solution:**
```cpp
class AutoNodeManager : public QObject
{
    Q_OBJECT

private:
    QNetworkAccessManager* m_networkManager;
    std::vector<QString> m_knownSeedNodes;

public:
    explicit AutoNodeManager(QObject* parent = nullptr);
    
private:
    void shuffleSeedNodes();  // Use std::shuffle instead
    void makeNetworkRequest(const QString& url);
};
```

**Key Changes:**
1. Replace `std::random_shuffle` with `std::shuffle` and `std::random_device`
2. Fix QNetworkRequest initialization with proper parentheses
3. Remove unused variables or mark them as `[[maybe_unused]]`
4. Update ClientModel method calls to use available APIs

### ClientModel Integration

**Current State Analysis:**
- `getVerificationProgress()` method doesn't exist
- `getNetworkActive()` method doesn't exist
- Available: `numBlocksChanged` signal with verification progress parameter

**Design Solution:**
```cpp
// Instead of direct method calls, use signal connections
connect(clientModel, &ClientModel::numBlocksChanged,
        this, [this](int count, const QDateTime& blockDate, const QString& blockHash, 
                     double nVerificationProgress, bool header, SynchronizationState sync_state) {
    if (!header) {
        m_bootstrapProgress = static_cast<int>(nVerificationProgress * 100);
        Q_EMIT bootstrapProgress(m_bootstrapProgress);
    }
});
```

## Data Models

### Bootstrap Progress Tracking

```cpp
struct BootstrapState {
    int progress;           // 0-100 percentage
    bool isActive;         // Bootstrap operation status
    QString currentSource; // Current download source
    qint64 bytesReceived;  // Downloaded bytes
    qint64 bytesTotal;     // Total bytes to download
};
```

### Node Discovery State

```cpp
struct NodeDiscoveryState {
    QStringList availableNodes;    // Discovered nodes
    QStringList connectedNodes;    // Currently connected nodes
    int networkHealthScore;        // 0-100 network quality score
    bool autoDiscoveryEnabled;     // Feature toggle
};
```

## Error Handling

### Network Request Error Handling

```cpp
void AutoBootstrapManager::handleNetworkError(QNetworkReply::NetworkError error)
{
    QString errorMessage;
    switch (error) {
        case QNetworkReply::ConnectionRefusedError:
            errorMessage = tr("Connection refused to bootstrap server");
            break;
        case QNetworkReply::HostNotFoundError:
            errorMessage = tr("Bootstrap server not found");
            break;
        default:
            errorMessage = tr("Network error: %1").arg(static_cast<int>(error));
    }
    Q_EMIT bootstrapError(errorMessage);
}
```

### Compilation Error Prevention

1. **Header Guards**: Ensure all headers have proper include guards
2. **Forward Declarations**: Use forward declarations to reduce compilation dependencies
3. **Qt MOC**: Ensure Q_OBJECT macro is properly placed and moc files are generated
4. **API Compatibility**: Use Qt version checks where necessary

## Testing Strategy

### Unit Testing Approach

1. **Mock ClientModel**: Create mock ClientModel for testing AutoBootstrap/AutoNode managers
2. **Network Mocking**: Use QNetworkAccessManager mocking for network request testing
3. **Signal Testing**: Verify signal emissions with correct parameters
4. **Qt Test Framework**: Use Qt's built-in testing framework

### Integration Testing

1. **Compilation Tests**: Ensure code compiles with Qt 6.9.3
2. **Runtime Tests**: Verify functionality works as expected
3. **Memory Leak Tests**: Check for proper QObject parent-child relationships
4. **Performance Tests**: Ensure no performance regression from changes

### Test Coverage Areas

- QObject inheritance and destruction
- Signal/slot connections and emissions
- Network request handling
- Error condition handling
- Bootstrap progress tracking
- Node discovery functionality

## Implementation Notes

### Qt 6 Specific Considerations

1. **QNetworkRequest**: Must be properly initialized with parentheses
2. **Signal Connections**: Require proper context objects
3. **Random Number Generation**: Use `<random>` header instead of deprecated algorithms
4. **String Conversions**: Use Qt string conversion methods consistently

### Backward Compatibility

- Maintain existing signal signatures for API compatibility
- Keep public method interfaces unchanged
- Preserve configuration file formats
- Maintain network protocol compatibility

### Performance Considerations

- Minimize object allocations in frequently called methods
- Use move semantics where appropriate
- Cache frequently accessed data
- Optimize network request batching