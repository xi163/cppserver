#操作系统版本 Linux/centOS7
#cat /etc/centos-release
	CentOS Linux release 7.6.1810 (Core)
#uname -sr
	Linux 3.10.0-957.el7.x86_64

#CPU型号
#cat /proc/cpuinfo| grep name| cut -f2 -d:| uniq -c
	4  Intel(R) Core(TM) i5-8500 CPU @ 3.00GHz

#dmidecode -s processor-version
Intel(R) Xeon(R) CPU E3-1220 v6 @ 3.00GHz

物理CPU个数(性能测试要求8+)
#cat /proc/cpuinfo| grep "physical id"| sort| uniq| wc -l
4

CPU核心数
#cat /proc/cpuinfo| grep "cpu cores"| uniq
cpu cores	: 1

逻辑CPU的个数(线程数，性能测试要求32+)
#cat /proc/cpuinfo| grep "processor"| wc -l
4
#cat /proc/cpuinfo | grep "siblings"
siblings	: 1
siblings	: 1
siblings	: 1
siblings	: 1

总核数 4 * 1 = 4核
总逻辑数 4 * 1 * 4 = 16逻辑单元

#cat /proc/cpuinfo

#lscpu
Architecture:          x86_64
CPU op-mode(s):        32-bit, 64-bit
Byte Order:            Little Endian
CPU(s):                4
On-line CPU(s) list:   0-3
Thread(s) per core:    1
Core(s) per socket:    1
Socket(s):             4
NUMA node(s):          1
Vendor ID:             GenuineIntel
CPU family:            6
Model:                 158
Model name:            Intel(R) Xeon(R) CPU E3-1220 v6 @ 3.00GHz
Stepping:              9
CPU MHz:               3000.000
BogoMIPS:              6000.00
Virtualization:        VT-x
Hypervisor vendor:     VMware
Virtualization type:   full
L1d cache:             32K
L1i cache:             32K
L2 cache:              256K
L3 cache:              8192K
NUMA node0 CPU(s):     0-3

网卡
#ethtool ens192
Settings for eth0:
	Supported ports: [ TP ]
	Supported link modes:   1000baseT/Full 
	                        10000baseT/Full 
	Supported pause frame use: No
	Supports auto-negotiation: No
	Supported FEC modes: Not reported
	Advertised link modes:  Not reported
	Advertised pause frame use: No
	Advertised auto-negotiation: No
	Advertised FEC modes: Not reported
	Speed: 10000Mb/s
	Duplex: Full
	Port: Twisted Pair
	PHYAD: 0
	Transceiver: internal
	Auto-negotiation: off
	MDI-X: Unknown
	Supports Wake-on: uag
	Wake-on: d
	Link detected: yes