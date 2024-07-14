# C++ Stock Ticker on Led RGB Display

This project replicates real-time Led Stock Ticker display that can be found on various marketplaces:
https://www.etsy.com/uk/market/led_stock_ticker

## Features:
- Real-time prices on various subscribed items fetched from FinnHub API (Stocks, ETFs, Crypto).
- Displays a chart based on 1min candlesticks using the last received price and the data from the PostgreSQL database. 
- Shows today's day gain/loss in percentages.
- Displays logos.

## Hardware:
- Raspberry Pi 4B.
- Adafruit 64x32 RGB Matrix.
- Adafruit Bonnet.
- Power supply.

## Getting Started
1. Clone this repository.
2. Open the `Scripts/` directory and run the appropriate OS `Setup` script to generate project files.
3. Connect RGB Led Matrix to the Raspberry Pi 4 GPIO pins.
4. Modify the config file to subscribe for updates and write symbol to logo mapping.
5. Add the FinnHub API key to the config file.
6. Run make and then appropriate compiled binary.

## Sources
- [FinnHub API](https://finnhub.io) to retrieve real-time market prices.
- [rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix) library to work with the LED board

Prototype:
![image](https://github.com/user-attachments/assets/45b43189-f218-42c4-bcec-dc8e10bd6f71)

## License
- Premake is licensed under BSD 3-Clause (see included LICENSE.txt file for more details)
