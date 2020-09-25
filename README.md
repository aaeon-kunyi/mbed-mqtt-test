![](./resources/official_armmbed_example_badge.png)
# Mbed OS with MQTT example

The example project is 
1. [Install Mbed CLI](https://os.mbed.com/docs/mbed-os/latest/quick-start/offline-with-mbed-cli.html).
2. From the command-line, import the example: `mbed import https://github.com/aaeon-kunyi/mbed-mqtt-test`
3. Change the current directory to where the project was imported.

## Application functionality

The `main()` function is the single thread in the application. It toggles the state of a digital output connected to an LED on the board.
and accept press user button 1 to publish mqtt message to test.mosquitto.org

## Building and running

1. Connect a USB cable between the USB port on the board and the host computer.
1. Run the following command to build the example project and program the microcontroller flash memory:

    ```bash
    $ mbed compile -m <TARGET> -t <TOOLCHAIN> --flash
    ```

Your PC may take a few minutes to compile your code.

The binary is located at `./BUILD/<TARGET>/<TOOLCHAIN>/mbed-mqtt-test.bin`.

Alternatively, you can manually copy the binary to the board, which you mount on the host computer over USB.

Depending on the target, you can build the example project with the `GCC_ARM`, `ARM` or `IAR` toolchain. After installing Arm Mbed CLI, run the command below to determine which toolchain supports your target:

```bash
$ mbed compile -S
```

## Expected output
The LED on your target turns on and off every 500 milliseconds.


## Troubleshooting
If you have problems, you can review the [documentation](https://os.mbed.com/docs/latest/tutorials/debugging.html) for suggestions on what could be wrong and how to fix it.

## Related Links

* [Mbed OS Stats API](https://os.mbed.com/docs/latest/apis/mbed-statistics.html).
* [Mbed OS Configuration](https://os.mbed.com/docs/latest/reference/configuration.html).
* [Mbed OS Serial Communication](https://os.mbed.com/docs/latest/tutorials/serial-communication.html).
* [Mbed OS bare metal](https://os.mbed.com/docs/mbed-os/latest/reference/mbed-os-bare-metal.html).
* [Mbed boards](https://os.mbed.com/platforms/).

### License and contributions

The software is provided under Apache-2.0 license. Contributions to this project are accepted under the same license. Please see contributing.md for more info.

This project contains code from other projects. The original license text is included in those source files. They must comply with our license guide.
