# BabaChain Core version v1.0.0

This is a new minor version release, bringing various bugfixes and performance improvements.
This release is **optional** for all nodes, although recommended.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/baba-chain/babachain/issues>


# Upgrading and downgrading

## How to Upgrade

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/BabaChain-Qt (on Mac) or
babachaind/babachain-qt (on Linux).

## Downgrade warning

### Downgrade to a version < v1.1.0

Downgrading to a version older than v1.1.0 may not be supported, and will
likely require a reindex.

# Release Notes

Bug Fixes
----------

- Fixed crash when processing invalid masternode payment destinations, replacing unsafe assertion with proper error handling (babachain#6740).

RPC and Logging Improvements
----------------------------

- Fixed misleading error logs that were triggered by legitimate RPC queries for non-existent transaction data, reducing log noise and preventing false alarms (babachain#6744).

Performance Improvements
------------------------

- Optimized versionbits calculation to avoid unnecessary computations during block operations, significantly improving performance during blockchain reorganizations (babachain#6632).

Documentation Updates
---------------------

- Updated translation documentation with current Transifex links and fixed typos to help contributors properly access translation resources (babachain#6739).



# Credits

Thanks to everyone who directly contributed to this release:

As well as everyone that submitted issues, reviewed pull requests and helped
debug the release candidates.

# Older releases

These releases are considered obsolete. Old release notes can be found here:

- [v1.0.0](https://github.com/baba-chain/babachain/blob/master/doc/release-notes/babachain/

[set-of-changes]: https://github.com/baba-chain/babachain/compare/v1.0.0...baba-chain:v1.0.1