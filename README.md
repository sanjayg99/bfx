# BFX Specs
* Algorithm - Scrypt PoS
* Premine - 120 million
* Decimals - 8
* Blocktime - 60 seconds
* Staking - Min to Max (2-23 days)
* Staking percent - 12% p.a.
* P2P Port- 6325
* RPC Port- 6326

# Open Source Repository for BFX Nodes & Wallets
More information here: https://bfxworld.com

Alpha Builds for commits that pass testing can be found here:
https://github.com/bfxcrypto/bfx/releases

Pull Requests Welcome

# Building BFX
```
git clone https://github.com/bfxcrypto/bfx.git && cd bfx
```

## Linux (debian-based)
### Install the following dependencies
```
sudo apt-get update && sudo apt-get install qt5-default qt5-qmake qtbase5-dev-tools \
qttools5-dev-tools build-essential libssl-dev libdb++-dev libminiupnpc-dev \
libqrencode-dev libcurl4-openssl-dev libldap2-dev libidn11-dev librtmp-dev libsodium-dev python -y
```

### Build OpenSSL, cURL, QREncode, and Boost
```
python ./build_scripts/CompileOpenSSL-Linux.py
python ./build_scripts/CompileCurl-Linux.py
python ./build_scripts/CompileQREncode-Linux.py
python ./build_scripts/CompileBoost-Linux.py
export PKG_CONFIG_PATH=$PWD/curl_build/lib/pkgconfig/
export OPENSSL_INCLUDE_PATH=$PWD/openssl_build/include/
export OPENSSL_LIB_PATH=$PWD/openssl_build/lib/
export BOOST_INCLUDE_PATH=$PWD/boost_build/include/
export BOOST_LIB_PATH=$PWD/boost_build/lib/
export QRENCODE_INCLUDE_PATH=$PWD/qrencode_build/include/
export QRENCODE_LIB_PATH=$PWD/qrencode_build/lib/
```

### Build bfxd
```
cd wallet
make "STATIC=1" -B -w -f makefile.unix -j4
strip ./bfxd
```

### Build BFX-Qt
```
qmake "USE_UPNP=1" "USE_QRCODE=1" "RELEASE=1" "QRENCODE_INCLUDE_PATH=$QRENCODE_INCLUDE_PATH" "QRENCODE_LIB_PATH=$QRENCODE_LIB_PATH" "BOOST_INCLUDE_PATH=$BOOST_INCLUDE_PATH" "BOOST_LIB_PATH=$BOOST_LIB_PATH" "OPENSSL_INCLUDE_PATH=$OPENSSL_INCLUDE_PATH" "OPENSSL_LIB_PATH=$OPENSSL_LIB_PATH" "PKG_CONFIG_PATH=$PKG_CONFIG_PATH" ../bfx-wallet.pro
make -B -w -j4
```

## Windows
We cross compile the windows binary on Linux using MXE (Ubuntu recommended)

### Download our pre-compiled MXE toolchain
```
https://files.nebl.io/dependencies/parts=12/mxe.tar.gz
tar -xf mxe.tar.gz
sudo mv mxe /
export PATH=/mxe/mxe/usr/bin:$PATH
```

### Build BFX-Qt 32 bit
```
i686-w64-mingw32.static-qmake-qt5 "USE_UPNP=1" "USE_QRCODE=1" "RELEASE=1" bfx-wallet.pro
make -B -w -j4
```

## macOS
### Install the following dependencies (homebrew)
```
brew update
brew fetch --retry qt            && brew install qt --force
brew fetch --retry berkeley-db@4 && brew install berkeley-db@4 --force
brew fetch --retry boost         && brew install boost --force
brew fetch --retry miniupnpc     && brew install miniupnpc --force
brew fetch --retry curl          && brew install curl --force
brew fetch --retry openssl@1.1   && brew install openssl@1.1 --force
brew fetch --retry qrencode      && brew install qrencode --force
brew fetch --retry libsodium     && brew install libsodium --force

brew unlink qt            && brew link --force --overwrite qt
brew unlink berkeley-db@4 && brew link --force --overwrite berkeley-db@4
brew unlink boost         && brew link --force --overwrite boost
brew unlink miniupnpc     && brew link --force --overwrite miniupnpc
brew unlink curl          && brew link --force --overwrite curl
brew unlink python        && brew link --force --overwrite python
brew unlink openssl@1.1   && brew link --force --overwrite openssl@1.1
brew unlink qrencode      && brew link --force --overwrite qrencode
brew unlink libsodium     && brew link --force --overwrite libsodium
```

### Build BFX-Qt
```
qmake "USE_UPNP=1" "USE_QRCODE=1" "RELEASE=1" bfx-wallet.pro
make -B -w -j4
```

### Prepare the .dmg file for release (optional)
```
./contrib/macdeploy/macdeployqtplus ./BFX-Qt.app -add-qt-tr da,de,es,hu,ru,uk,zh_CN,zh_TW -verbose 1 -rpath /usr/local/opt/qt/lib
appdmg ./contrib/macdeploy/appdmg.json ./BFX-Qt.dmg
```


# BFX Command Line Parameters
These parameters can be passed to bfxd or BFX-Qt at runtime. They can also be hardcoded in a bfx.conf file located in the data directory.

```
Usage:
  bfxd [options]
  bfxd [options] <command> [params]  Send command to -server or bfxd
  bfxd [options] help                List commands
  bfxd [options] help <command>      Get help for a command

Options:
  -?                     This help message
  -conf=<file>           Specify configuration file (default: bfx.conf)
  -pid=<file>            Specify pid file (default: bfxd.pid)
  -datadir=<dir>         Specify data directory
  -wallet=<dir>          Specify wallet file (within data directory)
  -dbcache=<n>           Set database cache size in megabytes (default: 25)
  -dblogsize=<n>         Set database disk log size in megabytes (default: 100)
  -timeout=<n>           Specify connection timeout in milliseconds (default: 5000)
  -proxy=<ip:port>       Connect through socks proxy
  -socks=<n>             Select the version of socks proxy to use (4-5, default: 5)
  -tor=<ip:port>         Use proxy to reach tor hidden services (default: same as -proxy)
  -dns                   Allow DNS lookups for -addnode, -seednode and -connect
  -port=<port>           Listen for connections on <port> (default: 6325 or testnet: 16325)
  -maxconnections=<n>    Maintain at most <n> connections to peers (default: 125)
  -addnode=<ip>          Add a node to connect to and attempt to keep the connection open
  -connect=<ip>          Connect only to the specified node(s)
  -seednode=<ip>         Connect to a node to retrieve peer addresses, and disconnect
  -externalip=<ip>       Specify your own public address
  -onlynet=<net>         Only connect to nodes in network <net> (IPv4, IPv6 or Tor)
  -discover              Discover own IP address (default: 1 when listening and no -externalip)
  -listen                Accept connections from outside (default: 1 if no -proxy or -connect)
  -bind=<addr>           Bind to given address. Use [host]:port notation for IPv6
  -dnsseed               Find peers using DNS lookup (default: 1)
  -staking               Stake your coins to support network and gain reward (default: 1)
  -synctime              Sync time with other nodes. Disable if time on your system is precise e.g. syncing with Network Time Protocol (default: 1)
  -cppolicy              Sync checkpoints policy (default: strict)
  -banscore=<n>          Threshold for disconnecting misbehaving peers (default: 100)
  -bantime=<n>           Number of seconds to keep misbehaving peers from reconnecting (default: 86400)
  -maxreceivebuffer=<n>  Maximum per-connection receive buffer, <n>*1000 bytes (default: 5000)
  -maxsendbuffer=<n>     Maximum per-connection send buffer, <n>*1000 bytes (default: 1000)
  -upnp                  Use UPnP to map the listening port (default: 1 when listening)
  -paytxfee=<amt>        Fee per KB to add to transactions you send
  -mininput=<amt>        When creating transactions, ignore inputs with value less than this (default: 0.01)
  -daemon                Run in the background as a daemon and accept commands
  -testnet               Use the test network
  -debug                 Output extra debugging information. Implies all other -debug* options
  -debugnet              Output extra network debugging information
  -noquicksync           If enabled, do not use quicksync, instead use regular sync
  -logtimestamps         Prepend debug output with timestamp
  -shrinkdebugfile       Shrink debug.log file on client startup (default: 1 when no -debug)
  -printtoconsole        Send trace/debug info to console instead of debug.log file
  -rpcuser=<user>        Username for JSON-RPC connections
  -rpcpassword=<pw>      Password for JSON-RPC connections
  -rpcport=<port>        Listen for JSON-RPC connections on <port> (default: 6326 or testnet: 16326)
  -rpcallowip=<ip>       Allow JSON-RPC connections from specified IP address
  -rpcconnect=<ip>       Send commands to node running on <ip> (default: 127.0.0.1)
  -blocknotify=<cmd>     Execute command when the best block changes (%s in cmd is replaced by block hash)
  -walletnotify=<cmd>    Execute command when a wallet transaction changes (%s in cmd is replaced by TxID)
  -confchange            Require a confirmations for change (default: 0)
  -enforcecanonical      Enforce transaction scripts to use canonical PUSH operators (default: 1)
  -alertnotify=<cmd>     Execute command when a relevant alert is received (%s in cmd is replaced by message)
  -upgradewallet         Upgrade wallet to latest format
  -keypool=<n>           Set key pool size to <n> (default: 100)
  -rescan                Rescan the block chain for missing wallet transactions
  -salvagewallet         Attempt to recover private keys from a corrupt wallet.dat
  -checkblocks=<n>       How many blocks to check at startup (default: 2500, 0 = all)
  -checklevel=<n>        How thorough the block verification is (0-6, default: 1)
  -loadblock=<file>      Imports blocks from external blk000?.dat file

Block creation options:
  -blockminsize=<n>      Set minimum block size in bytes (default: 0)
  -blockmaxsize=<n>      Set maximum block size in bytes (default: 8000000)
  -blockprioritysize=<n> Set maximum size of high-priority/low-fee transactions in bytes (default: 27000)

SSL options:
  -rpcssl                                  Use OpenSSL (https) for JSON-RPC connections
  -rpcsslcertificatechainfile=<file.cert>  Server certificate file (default: server.cert)
  -rpcsslprivatekeyfile=<file.pem>         Server private key (default: server.pem)
  -rpcsslciphers=<ciphers>                 Acceptable ciphers (default: TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!AH:!3DES:@STRENGTH)
```


# BFX RPC Commands
RPC commands are used to interact with a running instance of bfxd or BFX-Qt. They are used via the command line with bfxd, or via the BFX-Qt Debug Console.

```
addmultisigaddress <nrequired> <'["key","key"]'> [account]
addredeemscript <redeemScript> [account]
backupwallet <destination>
checkwallet
createrawbfxttransaction [{"txid":txid,"vout":n},...] {address:{tokenid/tokenName:tokenAmount},address:neblAmount,...} '{"userData":{"meta":[{"K1":"V1"},{},...]}}' [encrypt-metadata=false]
createrawtransaction [{"txid":txid,"vout":n},...] {address:amount,...}
decoderawtransaction <hex string> [ignoreBFXT=false]
decodescript <hex string>
dumpprivkey <bfxaddress>
dumpwallet <filename>
encryptwallet <passphrase>
exportblockchain <path-dir>
getaccount <bfxaddress>
getaccountaddress <account>
getaddressesbyaccount <account>
getbalance [account] [minconf=1]
getbestblockhash
getblock <hash> [verbose=true] [showtxns=false] [ignoreBFXT=false]
getblockbynumber <number> [txinfo] [ignoreBFXT=false]
getblockcount
getblockhash <index>
getblocktemplate [params]
getcheckpoint
getconnectioncount
getdifficulty
getinfo
getmininginfo
getnewaddress [account]
getnewpubkey [account]
getbfxtbalance <tokenId/name> [minconf=1]
getbfxtbalances [minconf=1]
getpeerinfo
getrawmempool
getrawtransaction <txid> [verbose=0] [ignoreBFXT=false]
getreceivedbyaccount <account> [minconf=1]
getreceivedbyaddress <bfxaddress> [minconf=1]
getstakinginfo
getsubsidy [nTarget]
gettransaction <txid> [ignoreBFXT=false]
getwork [data]
getworkex [data, coinbase]
help [command]
importprivkey <bfxprivkey> [label]
importwallet <filename>
keypoolrefill [new-size]
listaccounts [minconf=1]
listaddressgroupings
listreceivedbyaccount [minconf=1] [includeempty=false]
listreceivedbyaddress [minconf=1] [includeempty=false]
listsinceblock [blockhash] [target-confirmations]
listtransactions [account] [count=10] [from=0]
listunspent [minconf=1] [maxconf=9999999] ["address",...]
makekeypair [prefix]
move <fromaccount> <toaccount> <amount> [minconf=1] [comment]
repairwallet
resendtx
reservebalance [<reserve> [amount]]
sendalert <message> <privatekey> <minver> <maxver> <priority> <id> [cancelupto]
sendfrom <fromaccount> <tobfxaddress> <amount> [minconf=1] [comment] [comment-to]
sendmany <fromaccount (must be empty, unsupported)> {address:amount,...} [comment]
sendbfxttoaddress <bfxaddress> <amount> <tokenId/tokenName> '{"userData":{"meta":[{"K1":"V1"},{},...]}}' [encrypt-metadata=false] [comment] [comment-to]
sendrawtransaction <hex string>
sendtoaddress <bfxaddress> <amount> [comment] [comment-to]
setaccount <bfxaddress> <account>
settxfee <amount>
signmessage <bfxaddress> <message>
signrawtransaction <hex string> [{"txid":txid,"vout":n,"scriptPubKey":hex},...] [<privatekey1>,...] [sighashtype="ALL"]
stop
submitblock <hex data> [optional-params-obj]
udtobfxaddress <unstoppable domain address>
validateaddress <bfxaddress>
validatepubkey <bfxpubkey>
verifymessage <bfxaddress> <signature> <message>
```

# QT & bfxd Configuring 
The configuration file is a list of setting=value pairs, one per line, with optional comments starting with the '#' character. The configuration file is not automatically created; you can create it using your favorite plain-text editor. By default, BFX (or bfxd) will look for a file named 'bfx.conf' in the BFX data directory, but both the data directory and the configuration file path may be changed using the -datadir and -conf command-line arguments.
```
Operating System        Default bitcoin datadir                    Typical path to configuration file
Windows	                %APPDATA%\bfx\	                           C:\Users\username\AppData\Roaming\bfx\bfx.conf
Linux	                  $HOME/.bfx/	                               /home/username/.bfx/bfx.conf
Mac OSX	                $HOME/Library/Application Support/bfx/	   /Users/username/Library/Application Support/bfx/bfx.conf
```

bfx.conf example can be found here: https://github.com/bfxcrypto/bfx/blob/master/doc/bfx.conf

# Responsible Vulnerability Disclosure
If you believe you've found a security issue in our product or service, we encourage you to notify us. We welcome working with skilled security researchers across the globe to resolve any issues promptly.

This Bug Bounty program is an open offer to external individuals to receive compensation for reporting BFX bugs, specifically related to security of the core functionality of the network.

## Submissions
Submissions should be made through the following [contact page](https://bfxworld.com/contact)

Include following in your report:
1. Severity - Your opinion on the severity of the issue (e.g. high, moderate, low)
2. Summary - Add summary of the vulnerability
3. Description - Any additional details about this vulnerability
4. Steps - Steps to reproduce
5. Supporting Material/References - Source code to replicate, list any additional material (e.g. screenshots, logs, etc.)
6. Impact - What security impact could an attacker achieve?
7. Your name and country.

Please be available to cooperate with the BFX Team to provide further information on the bug if needed.

## Rewards
Rewards are at the discretion of BFX, and we will not be awarding significant bounties for low severity bugs.

## Examples of eligible bugs
### Critical
- bugs which can take full control of BFX nodes.
- bugs which can lead to private key leakage.
- bugs which can lead to unauthorised transfer or generation of coins/BFXT tokens.
### High
- bugs which can incur Denial of Service (DoS) in the BFX network through P2P network.
### Medium
- bugs which can incur Denial of Service (DoS) in the BFX network through the RPC-API.
- bugs allowing unauthorized operations on user accounts.

## Program Rules
- Vulnerabilities relating to this repository, the core software running the BFX Network, are eligible for this program. Out-of-scope vulnerabilities impacting other software we release may be considered under this program at BFX's discretion.
- As this is a private program, please do not discuss this program or any vulnerabilities (even resolved ones) outside of the program without express consent from the organization.
- Please provide detailed reports with reproducible steps. If the report is not detailed enough to reproduce the issue, the issue will not be eligible for a reward.
- Submit vulnerabilities only for the latest release, vulnerabilities submitted for older versions are not eligible for a reward.

Not following these rules will disqualify you from receiving any rewards.
