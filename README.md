<div align="center">
  <a href="https://ton.org">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="https://ton.org/download/ton_logo_dark_background.svg">
      <img alt="TON logo" src="https://ton.org/download/ton_logo_light_background.svg">
    </picture>
  </a>
  <h3>Reference implementation of TON Node and tools</h3>
  <hr/>
</div>

## 
[![TON Overflow Group][ton-overflow-badge]][ton-overflow-url]
[![Stack Overflow Group][stack-overflow-badge]][stack-overflow-url]
[![Telegram Community Chat][telegram-tondev-badge]][telegram-tondev-url]
[![Telegram Community Group][telegram-community-badge]][telegram-community-url]
[![Telegram Foundation Group][telegram-foundation-badge]][telegram-foundation-url]
[![Twitter Group][twitter-badge]][twitter-url]

[telegram-foundation-badge]: https://img.shields.io/badge/TON%20Foundation-2CA5E0?logo=telegram&logoColor=white&style=flat
[telegram-community-badge]: https://img.shields.io/badge/TON%20Community-2CA5E0?logo=telegram&logoColor=white&style=flat
[telegram-tondev-badge]: https://img.shields.io/badge/chat-TONDev-2CA5E0?logo=telegram&logoColor=white&style=flat
[telegram-foundation-url]: https://t.me/tonblockchain
[telegram-community-url]: https://t.me/toncoin
[telegram-tondev-url]: https://t.me/tondev_eng
[twitter-badge]: https://img.shields.io/twitter/follow/ton_blockchain
[twitter-url]: https://twitter.com/ton_blockchain
[stack-overflow-badge]: https://img.shields.io/badge/-Stack%20Overflow-FE7A16?style=flat&logo=stack-overflow&logoColor=white
[stack-overflow-url]: https://stackoverflow.com/questions/tagged/ton
[ton-overflow-badge]: https://img.shields.io/badge/-TON%20Overflow-FE7A16?style=flat&logo=stack-overflow&logoColor=white
[ton-overflow-url]: https://answers.ton.org



Main TON monorepo, which includes the code of the node/validator, lite-client, tonlib, FunC compiler, etc.

## The Open Network

__The Open Network (TON)__ is a fast, secure, scalable blockchain focused on handling _millions of transactions per second_ (TPS) with the goal of reaching hundreds of millions of blockchain users.
- To learn more about different aspects of TON blockchain and its underlying ecosystem check [documentation](https://ton.org/docs)
- To run node, validator or lite-server check [Participate section](https://ton.org/docs/participate/nodes/run-node)
- To develop decentralised apps check [Tutorials](https://ton.org/docs/develop/smart-contracts/), [FunC docs](https://ton.org/docs/develop/func/overview) and [DApp tutorials](https://ton.org/docs/develop/dapps/)
- To work on TON check [wallets](https://ton.app/wallets), [explorers](https://ton.app/explorers), [DEXes](https://ton.app/dex) and [utilities](https://ton.app/utilities)
- To interact with TON check [APIs](https://ton.org/docs/develop/dapps/apis/)

## Get TON blockchain precompiled executables
TON distributes precompiled executables via various package managers.
All distribution packages contain the executables from the latest TON release. 

<details>
<summary>
Click here to reveal the list of installed binaries and their locations.
</summary>

#### For Linux systems: 

**/usr/bin** 

* validator-engine-console
* validator-engine
* tonlib-cli
* tlbc
* storage-daemon-cli
* storage-daemon
* storage-cli
* rldp-http-proxy
* lite-client
* http-proxy
* generate-random-id
* func
* fift
* dht-server
* create-state
* create-hardfork
* blockchain-explorer
* adnl-proxy


**/usr/lib**
* libtonlibjson.so
* libemulator.so

**/usr/lib/fift**
* TonUtil.fif
* Stack.fif
* Lists.fif
* Lisp.fif
* GetOpt.fif
* Fift.fif
* FiftExt.fif
* Disasm.fif
* Color.fif
* Asm.fif

**/usr/share/ton/smartcont/**

* wallet-v3.fif
* wallet-v3-code.fif
* wallet-v2.fif
* wallet.fif
* wallet-code.fc
* wallet3-code.fc
* validator-elect-signed.fif
* validator-elect-req.fif
* update-elector-smc.fif
* update-config-smc.fif
* update-config.fif
* testgiver.fif
* stdlib.fc
* simple-wallet-ext-code.fc
* simple-wallet-code.fc
* show-addr.fif
* restricted-wallet-code.fc
* restricted-wallet3-code.fc
* restricted-wallet2-code.fc
* recover-stake.fif
* pow-testgiver-code.fc
* payment-channel-code.fc
* new-wallet-v3.fif
* new-wallet-v2.fif
* new-wallet.fif
* new-testgiver.fif
* new-restricted-wallet.fif
* new-restricted-wallet3.fif
* new-restricted-wallet2.fif
* new-pow-testgiver.fif
* new-pinger.fif
* new-manual-dns.fif
* new-highload-wallet-v2.fif
* new-highload-wallet.fif
* new-auto-dns.fif
* multisig-code.fc
* mathlib.fc
* manual-dns-manage.fif
* LICENSE.LGPL
* highload-wallet-v2-one.fif
* highload-wallet-v2.fif
* highload-wallet-v2-code.fc
* highload-wallet.fif
* highload-wallet-code.fc
* gen-zerostate-test.fif
* gen-zerostate.fif
* envelope-complaint.fif
* elector-code.fc
* dns-manual-code.fc
* dns-auto-code.fc
* CreateState.fif
* create-elector-upgrade-proposal.fif
* create-config-upgrade-proposal.fif
* create-config-proposal.fif
* config-proposal-vote-signed.fif
* config-proposal-vote-req.fif
* config-code.fc
* complaint-vote-signed.fif
* complaint-vote-req.fif
* auto-dns.fif
* asm-to-cpp.fif

 **/usr/share/ton/smartcont/auto**

* config-code.cpp
* config-code.fif
* dns-auto-code.cpp
* dns-auto-code.fif
* dns-manual-code.cpp
* dns-manual-code.fif
* elector-code.cpp
* elector-code.fif
* highload-wallet-code.cpp
* highload-wallet-code.fif
* highload-wallet-v2-code.cpp
* highload-wallet-v2-code.fif
* multisig-code.cpp
* multisig-code.fif
* payment-channel-code.cpp
* payment-channel-code.fif
* pow-testgiver-code.cpp
* pow-testgiver-code.fif
* restricted-wallet-code.cpp
* restricted-wallet-code.fif
* restricted-wallet2-code.cpp
* restricted-wallet2-code.fif
* restricted-wallet3-code.cpp
* restricted-wallet3-code.fif
* simple-wallet-code.cpp
* simple-wallet-code.fif
* simple-wallet-ext-code.cpp
* simple-wallet-ext-code.fif
* wallet-code.cpp
* wallet-code.fif
* wallet3-code.cpp
* wallet3-code.fif

#### For MacOS systems:

Depending on your macOS version, either here 
* /usr/local/bin
* /usr/local/lib
* /usr/local/lib/fift
* /usr/local/share/ton/ton/smartcont
* /usr/local/share/ton/ton/smartcont/auto
 
or here:

* /opt/homebrew/bin
* /opt/homebrew/lib
* /opt/homebrew/lib/fift
* /opt/homebrew/share/ton/ton/smartcont
* /opt/homebrew/share/ton/ton/smartcont/auto

#### For Windows systems:
Default locations under C:\ drive:

* C:\ProgramData\chocolatey\lib\ton\bin
* C:\ProgramData\chocolatey\lib\ton\bin\lib
* C:\ProgramData\chocolatey\lib\ton\bin\smartcont
* C:\ProgramData\chocolatey\lib\ton\bin\smartcont\auto

</details>

### Install deb (apt)
#### Debian, Ubuntu, Linux Mint... (x86-64, aarch64)
```
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys F6A649124520E5F3
sudo add-apt-repository ppa:ton-foundation/ppa
sudo apt update
sudo apt install ton
```

### Install via brew
#### macOS (x86-64, aarch64)
```
brew tap ton-blockchain/ton
brew install ton

# upgrade
brew update
brew reinstall ton
```

### Install via Chocolatey
#### Windows (x86-64)
Please, be aware that multiple false positive alarms from various antivirus vendors may occur.
This is an expected behaviour and there is no reason to worry.

Open an elevated terminal (Run as Administrator) and execute the below command:
```
choco install ton
```

### Install RPM (yum)
#### RedHat, Fedora, CentOS... (x86-64, aarch64)
```
sudo bash -c 'cat > /etc/yum.repos.d/ton.repo << EOF
[ton]
name=TON
baseurl=https://ton-blockchain.github.io/packages/rpm
enabled=1
type=rpm
gpgcheck=0
EOF'

sudo yum install -y ton
```

### Install AUR (pamac)
#### Manjaro, RebornOS, Arch Linux... (x86-64, aarch64)
```
sudo pamac build -no-confirm ton-bin
```

## Build TON blockchain from sources

Clone current repository and refer below to the section for your OS. 
```bash
git clone --recursive https://github.com/ton-blockchain/ton.git
cd ton
```

### Ubuntu 20.4, 22.04 (x86-64, aarch64)
Install additional system libraries
```bash
  sudo apt-get update
  sudo apt-get install -y build-essential git cmake ninja-build zlib1g-dev libsecp256k1-dev libmicrohttpd-dev libsodium-dev
          
  wget https://apt.llvm.org/llvm.sh
  chmod +x llvm.sh
  sudo ./llvm.sh 16 all
```
Compile TON binaries
```bash
  cp assembly/native/build-ubuntu-shared.sh .
  chmod +x build-ubuntu-shared.sh
  ./build-ubuntu-shared.sh  
```

### MacOS 11, 12 (x86-64, aarch64)
```bash
  cp assembly/native/build-macos-shared.sh .
  chmod +x build-macos-shared.sh
  ./build-macos-shared.sh
```

### Windows 10, 11, Server (x86-64)
You need to install `MS Visual Studio 2022` first.
Go to https://www.visualstudio.com/downloads/ and download `MS Visual Studio 2022 Community`.

Launch installer and select `Desktop development with C++`. 
After installation, also make sure that `cmake` is globally available by adding
`C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin` to the system `PATH` (adjust the path per your needs).

Open an elevated (Run as Administrator) `x86-64 Native Tools Command Prompt for VS 2022`, go to the root folder and execute: 
```bash
  copy assembly\native\build-windows.bat .
  build-windows.bat
```

### Building TON to WebAssembly
Install additional system libraries on Ubuntu
```bash
  sudo apt-get update
  sudo apt-get install -y build-essential git cmake ninja-build zlib1g-dev libsecp256k1-dev libmicrohttpd-dev libsodium-dev
          
  wget https://apt.llvm.org/llvm.sh
  chmod +x llvm.sh
  sudo ./llvm.sh 16 all
```
Compile TON binaries with emscripten
```bash
  cd assembly/wasm
  chmod +x fift-func-wasm-build-ubuntu.sh
  ./fift-func-wasm-build-ubuntu.sh
```

### Building TON tonlib library for Android (arm64-v8a, armeabi-v7a, x86, x86-64)
Install additional system libraries on Ubuntu
```bash
  sudo apt-get update
  sudo apt-get install -y build-essential git cmake ninja-build automake libtool texinfo autoconf libgflags-dev \
  zlib1g-dev libssl-dev libreadline-dev libmicrohttpd-dev pkg-config libgsl-dev python3 python3-dev \
  libtool autoconf libsodium-dev libsecp256k1-dev
```
Compile TON tonlib library
```bash
  cp assembly/android/build-android-tonlib.sh .
  chmod +x build-android-tonlib.sh
  ./build-android-tonlib.sh
```

### Build TON portable binaries with Nix package manager
You need to install Nix first.
```bash
   sh <(curl -L https://nixos.org/nix/install) --daemon
```
Then compile TON with Nix by executing below command from the root folder: 
```bash
  cp -r assembly/nix/* .
  export NIX_PATH=nixpkgs=https://github.com/nixOS/nixpkgs/archive/23.05.tar.gz
  nix-build linux-x86-64-static.nix
```
More examples for other platforms can be found under `assembly/nix`.  


## Running tests

Tests are executed by running `ctest` in the build directory. See `doc/Tests.md` for more information.

## Updates flow

* **master branch** - mainnet is running on this stable branch.

  Only emergency updates, urgent updates, or updates that do not affect the main codebase (GitHub workflows / docker images / documentation) are committed directly to this branch.

* **testnet branch** - testnet is running on this branch. The branch contains a set of new updates. After testing, the testnet branch is merged into the master branch and then a new set of updates is added to testnet branch.

* **backlog** - other branches that are candidates to getting into the testnet branch in the next iteration.

Usually, the response to your pull request will indicate which section it falls into.


## "Soft" Pull Request rules

* Thou shall not merge your own PRs, at least one person should review the PR and merge it (4-eyes rule)
* Thou shall make sure that workflows are cleanly completed for your PR before considering merge
