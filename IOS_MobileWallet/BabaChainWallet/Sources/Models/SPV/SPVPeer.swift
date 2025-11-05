//
//  SPVPeer.swift
//  BabaChainWallet
//
//  Created by BabaChain Core Group
//  Copyright © 2024 BabaChain Core Group. All rights reserved.
//
//  Licensed under the MIT License (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  https://opensource.org/licenses/MIT
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

import Foundation
import Network

protocol SPVPeerDelegate: AnyObject {
    func peer(_ peer: SPVPeer, didReceiveBlockHeader header: BlockHeader)
    func peer(_ peer: SPVPeer, didConnect: Bool)
    func peer(_ peer: SPVPeer, didDisconnect error: Error?)
}

class SPVPeer: NSObject {
    
    // MARK: - Properties
    
    let host: String
    let port: UInt16
    weak var delegate: SPVPeerDelegate?
    
    private var connection: NWConnection?
    private var isConnected = false
    private let queue = DispatchQueue(label: "SPVPeerQueue")
    
    // MARK: - Initialization
    
    init(host: String, port: UInt16) {
        self.host = host
        self.port = port
        super.init()
    }
    
    // MARK: - Connection Management
    
    func connect() {
        guard connection == nil else { return }
        
        let endpoint = NWEndpoint.hostPort(host: NWEndpoint.Host(host), port: NWEndpoint.Port(integerLiteral: port))
        connection = NWConnection(to: endpoint, using: .tcp)
        
        connection?.stateUpdateHandler = { [weak self] state in
            self?.handleConnectionState(state)
        }
        
        connection?.start(queue: queue)
        startReceiving()
    }
    
    func disconnect() {
        connection?.cancel()
        connection = nil
        isConnected = false
    }
    
    // MARK: - Message Handling
    
    func requestBlockHeaders(from startHeight: UInt32 = 0, count: UInt32 = 2000) {
        guard isConnected else { return }
        
        let message = BabaChainMessage.getHeaders(startHeight: startHeight, count: count)
        sendMessage(message)
    }
    
    func requestStakingInfo() {
        guard isConnected else { return }
        
        let message = BabaChainMessage.getStakingInfo()
        sendMessage(message)
    }
    
    // MARK: - Private Methods
    
    private func handleConnectionState(_ state: NWConnection.State) {
        switch state {
        case .ready:
            isConnected = true
            DispatchQueue.main.async {
                self.delegate?.peer(self, didConnect: true)
            }
            
        case .failed(let error):
            isConnected = false
            DispatchQueue.main.async {
                self.delegate?.peer(self, didDisconnect: error)
            }
            
        case .cancelled:
            isConnected = false
            DispatchQueue.main.async {
                self.delegate?.peer(self, didDisconnect: nil)
            }
            
        default:
            break
        }
    }
    
    private func startReceiving() {
        connection?.receive(minimumIncompleteLength: 1, maximumLength: 65536) { [weak self] data, _, isComplete, error in
            if let data = data, !data.isEmpty {
                self?.processReceivedData(data)
            }
            
            if let error = error {
                DispatchQueue.main.async {
                    self?.delegate?.peer(self!, didDisconnect: error)
                }
                return
            }
            
            if !isComplete {
                self?.startReceiving()
            }
        }
    }
    
    private func processReceivedData(_ data: Data) {
        // Parse BabaChain protocol messages
        guard let message = BabaChainMessage.parse(data) else { return }
        
        switch message.command {
        case "headers":
            if let headers = message.parseBlockHeaders() {
                for header in headers {
                    DispatchQueue.main.async {
                        self.delegate?.peer(self, didReceiveBlockHeader: header)
                    }
                }
            }
            
        case "stakinginfo":
            // Handle staking information response
            break
            
        default:
            break
        }
    }
    
    private func sendMessage(_ message: BabaChainMessage) {
        guard isConnected, let connection = connection else { return }
        
        let data = message.serialize()
        connection.send(content: data, completion: .contentProcessed { error in
            if let error = error {
                print("Failed to send message: \(error)")
            }
        })
    }
}

// MARK: - Hashable

extension SPVPeer: Hashable {
    static func == (lhs: SPVPeer, rhs: SPVPeer) -> Bool {
        return lhs.host == rhs.host && lhs.port == rhs.port
    }
    
    func hash(into hasher: inout Hasher) {
        hasher.combine(host)
        hasher.combine(port)
    }
}

// MARK: - BabaChain Network Configuration

struct BabaChainNetwork {
    static let seedNodes: [SeedNode] = [
        SeedNode(host: "seed1.babachain.org", port: 9999),
        SeedNode(host: "seed2.babachain.org", port: 9999),
        SeedNode(host: "seed3.babachain.org", port: 9999),
        SeedNode(host: "seed4.babachain.org", port: 9999)
    ]
    
    static let currentBlockHeight: Int = 1000000 // This would be dynamically updated
    
    static let networkMagic: UInt32 = 0xBD6B0CBF // BabaChain network magic bytes
}

struct SeedNode {
    let host: String
    let port: UInt16
}

// MARK: - BabaChain Protocol Messages

struct BabaChainMessage {
    let command: String
    let payload: Data
    
    static func getHeaders(startHeight: UInt32, count: UInt32) -> BabaChainMessage {
        var payload = Data()
        payload.append(contentsOf: withUnsafeBytes(of: startHeight.littleEndian) { Array($0) })
        payload.append(contentsOf: withUnsafeBytes(of: count.littleEndian) { Array($0) })
        
        return BabaChainMessage(command: "getheaders", payload: payload)
    }
    
    static func getStakingInfo() -> BabaChainMessage {
        return BabaChainMessage(command: "getstakinginfo", payload: Data())
    }
    
    static func parse(_ data: Data) -> BabaChainMessage? {
        // Parse BabaChain protocol message format
        guard data.count >= 24 else { return nil } // Minimum header size
        
        // Extract magic bytes, command, and payload
        let magic = data.subdata(in: 0..<4).withUnsafeBytes { $0.load(as: UInt32.self) }
        guard magic == BabaChainNetwork.networkMagic else { return nil }
        
        let commandData = data.subdata(in: 4..<16)
        let command = String(data: commandData, encoding: .utf8)?.trimmingCharacters(in: .nullCharacters) ?? ""
        
        let payloadLength = data.subdata(in: 16..<20).withUnsafeBytes { $0.load(as: UInt32.self) }
        let payload = data.subdata(in: 24..<min(data.count, Int(24 + payloadLength)))
        
        return BabaChainMessage(command: command, payload: payload)
    }
    
    func serialize() -> Data {
        var data = Data()
        
        // Magic bytes
        data.append(contentsOf: withUnsafeBytes(of: BabaChainNetwork.networkMagic.littleEndian) { Array($0) })
        
        // Command (12 bytes, null-padded)
        var commandBytes = Data(command.utf8)
        commandBytes.append(Data(repeating: 0, count: 12 - commandBytes.count))
        data.append(commandBytes)
        
        // Payload length
        let payloadLength = UInt32(payload.count)
        data.append(contentsOf: withUnsafeBytes(of: payloadLength.littleEndian) { Array($0) })
        
        // Checksum (simplified)
        let checksum = payload.sha256.prefix(4)
        data.append(checksum)
        
        // Payload
        data.append(payload)
        
        return data
    }
    
    func parseBlockHeaders() -> [BlockHeader]? {
        // Parse block headers from payload
        var headers: [BlockHeader] = []
        var offset = 0
        
        // Read number of headers
        guard payload.count >= 1 else { return nil }
        let headerCount = payload[0]
        offset += 1
        
        for _ in 0..<headerCount {
            guard offset + 80 <= payload.count else { break }
            
            let headerData = payload.subdata(in: offset..<offset + 80)
            if let header = parseBlockHeader(headerData) {
                headers.append(header)
            }
            
            offset += 80
        }
        
        return headers.isEmpty ? nil : headers
    }
    
    private func parseBlockHeader(_ data: Data) -> BlockHeader? {
        guard data.count >= 80 else { return nil }
        
        // Parse block header fields (simplified)
        let hash = data.sha256.hexString
        let previousHash = data.subdata(in: 4..<36).hexString
        let merkleRoot = data.subdata(in: 36..<68).hexString
        let timestamp = data.subdata(in: 68..<72).withUnsafeBytes { $0.load(as: UInt32.self) }
        
        // For now, we'll use a placeholder height - in a real implementation,
        // this would be calculated based on the chain
        let height = UInt32(0)
        
        return BlockHeader(
            hash: hash,
            previousHash: previousHash,
            merkleRoot: merkleRoot,
            timestamp: timestamp,
            height: height,
            stakingData: nil
        )
    }
}

// MARK: - Data Extensions

extension Data {
    var sha256: Data {
        // Simplified SHA256 - in production, use proper crypto library
        return self // Placeholder
    }
    
    var hexString: String {
        return map { String(format: "%02hhx", $0) }.joined()
    }
}