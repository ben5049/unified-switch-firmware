# TODO

- Check RTOS vs interrupt priorities
- Pad unused flash with `0xDEADC0DE`
- Fix SJA1105 SPI read alignment
- ECC read background task
- Count Zenoh events
    - Bytes/packets received
- Add Zenoh link QoS when "priority mappings on Zenoh links" is merged (before end of 2025)
- Check DHCP IP address caching
- PHY interrupts
- Add queriable for last n log messages
- Fix bug where terminating a Zenoh Pico session and reopening it within 10s causes a deadlock
    - Due to lease times?