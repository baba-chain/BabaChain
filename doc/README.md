BabaChain Core
==========

This is the official reference wallet for BabaChain digital currency and comprises the backbone of the BabaChain peer-to-peer network. BabaChain is a modern Proof-of-Stake cryptocurrency designed for energy efficiency, fair distribution, and sustainable rewards. You can [download BabaChain Core](https://www.babachain.org/downloads/) or [build it yourself](#building) using the guides below.

Running
---------------------
The following are some helpful notes on how to run BabaChain Core on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/babachain-qt` (GUI) or
- `bin/babachaind` (headless)

### Windows

Unpack the files into a directory, and then run babachain-qt.exe.

### macOS

Drag BabaChain Core to your applications folder, and then run BabaChain Core.

### Need Help?

* See the [BabaChain documentation](https://docs.babachain.org) for help and more information
* Ask for help on [BabaChain Discord](https://discord.gg/babachain)
* Ask for help on the [BabaChain Forum](https://forum.babachain.org)
* Check out the [Staking Guide](staking.md) to start earning rewards
* Read about [PoS Consensus](pos-consensus.md) for technical details

Building
---------------------
The following are developer notes on how to build BabaChain Core on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [Dependencies](dependencies.md)
- [macOS Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [NetBSD Build Notes](build-netbsd.md)
- [Android Build Notes](build-android.md)

Development
---------------------
The BabaChain Core repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Productivity Notes](productivity.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- Source Code Documentation ***TODO***
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [JSON-RPC Interface](JSON-RPC-interface.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)
- [Internal Design Docs](design/)

### Resources
* See the [BabaChain Developer Documentation](https://docs.babachain.org/developers/)
  for technical specifications and implementation details.
* Discuss on the [BabaChain Forum](https://forum.babachain.org), in the Development & Technical Discussion board.
* Discuss on [BabaChain Discord](https://discord.gg/babachain)
* Follow us on [Twitter](https://twitter.com/babachainorg) for updates

### BabaChain Specific
- [Staking Guide](staking.md)
- [PoS Consensus](pos-consensus.md)
- [Validator Setup](validator-setup.md)
- [Migration from Dash](migration-guide.md)

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [babachain.conf Configuration File](babachain-conf.md)
- [CJDNS Support](cjdns.md)
- [Files](files.md)
- [Fuzz-testing](fuzzing.md)
- [I2P Support](i2p.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)
- [Managing Wallets](managing-wallets.md)
- [Multisig Tutorial](multisig-tutorial.md)
- [P2P bad ports definition and list](p2p-bad-ports.md)
- [PSBT support](psbt.md)
- [Reduce Memory](reduce-memory.md)
- [Reduce Traffic](reduce-traffic.md)
- [Tor Support](tor.md)
- [Transaction Relay Policy](policy/README.md)
- [ZMQ](zmq.md)

License
---------------------
Distributed under the [MIT software license](/COPYING).
