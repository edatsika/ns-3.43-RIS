# ns-3.43-RIS

The ris-module is added in ns-3.4/contrib folder and enables the simulation of a an uplink scenario where users communicate with the BS via the associated RIS.
For each user, the RIS that offers the higher SNR is selected. A UDP client-server model is used, where users are configured as clients (UdpClientHelper) and the BS is configured as the server (UdpServerHelper).
A TDMA protocol is considered, i.e., the users transmit to the BS during their respective time slots, using the RIS to improve the channel conditions.

## License

This software is licensed under the terms of the GNU General Public License v2.0 only (GPL-2.0-only).
See the LICENSE file for more details.


## Building ns-3 with the ris-module

The following dependencies need to be installed:

```shell
sudo apt install g++ python3 python3-dev pkg-config sqlite3 qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools
```
Before building ns-3, you must configure it.
This step allows the configuration of the build options,
such as whether to enable the examples, tests and more.

To configure ns-3 with examples and tests enabled,
run the following command on the ns-3 main directory:

```shell
./ns3 configure --enable-examples --enable-tests
```

Then, build ns-3 by running the following command and redirect output to log file:

```shell
./ns3 build > build.log 2>&1
```

## Testing ns-3.43-RIS

To specify the user transmission power run the following command (default value: 20 dBm, example: 30 dBm):
```shell
export TX_POWER_DBM=30.0
```
Next, to run the ris-module-test-suite, run the following command on the ns-3 main directory:

```shell
./test.py -s ris-module -v
```







