# AmoveoMinerCpu

Amoveo Cryptocurrency Miner for Cpu work to be used with [AmoveoPool2.com](http://AmoveoPool2.com)


## Windows

### Releases

   [Latest pre-built releases are here](https://github.com/Mandelhoff/AmoveoMinerCpu/releases)


### Run
Usage Template:
```
AmoveoMinerCpu.exe {WalletAddress} {Threads} {RandomSeed} {PoolUrl}
```
* Threads is optional and defaults to 4.
* RandomSeed is optional. Set this if you are encountering many Duplicate Nonce errors.
* PoolUrl is optional and defaults to http://amoveopool2.com/work
    
Example Usage:  
```
AmoveoMinerCpu.exe BPA3r0XDT1V8W4sB14YKyuu/PgC6ujjYooVVzq1q1s5b6CAKeu9oLfmxlplcPd+34kfZ1qx+Dwe3EeoPu0SpzcI=
```

Example Usage with Threads and PoolUrl set:
```
AmoveoMinerCpu.exe BPA3r0XDT1V8W4sB14YKyuu/PgC6ujjYooVVzq1q1s5b6CAKeu9oLfmxlplcPd+34kfZ1qx+Dwe3EeoPu0SpzcI= 7 rAnDoMsTrInG http://amoveopool2.com/work
```

### Build
The Windows releases are built with Visual Studio 2015 with RestCPP, boost, and openSSL.




## Ubuntu/Linux

### Dependencies
```
sudo apt-get install libcpprest-dev libncurses5-dev libssl-dev unixodbc-dev g++ git
```

### Build
```
git clone https://github.com/Mandelhoff/AmoveoMinerCpu.git
cd AmoveoMinerCpu
sh build.sh
```

### Run
```
./AmoveoMinerCpu {WalletAddress} {Threads} {RandomSeed} {PoolUrl}
```
* Threads is optional and defaults to 4.
* RandomSeed is optional. Set this if you are encountering many Duplicate Nonce errors.
* PoolUrl is optional and defaults to http://amoveopool2.com/work





