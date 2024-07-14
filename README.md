# LED Stock Ticker on Raspberry Pi

This project replicates the functionality of real-time LED Stock Ticker displays commonly found in marketplaces like [Etsy Led Stock Ticker](https://www.etsy.com/uk/market/led_stock_ticker).

![Prototype](https://github.com/user-attachments/assets/45b43189-f218-42c4-bcec-dc8e10bd6f71)

## Features

- **Real-time Market Prices**: Fetches real-time prices for stocks, ETFs, and cryptocurrencies using the [FinnHub API](https://finnhub.io).
- **1-Minute Candlestick Charts**: Displays charts using the last received price and historical data stored in a PostgreSQL database.
- **Daily Gains/Losses**: Shows today's gain or loss in percentage.
- **Logos Display**: Displays logos for the subscribed items.
- **Visualization**: Visualization of the above features on the bright RGB LED display using GPIO pins interaction.

## Hardware Requirements

- Raspberry Pi 4B
- Adafruit 64x32 RGB Matrix
- Adafruit Bonnet
- Power Supply

## Getting Started

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/yahnyshc/stockTicker

2. **Setup the Environment**:

Navigate to the Scripts/ directory and run the appropriate setup script for your OS to generate project files.

4. **Connect Hardware**:
   
Connect the RGB LED Matrix to the Raspberry Pi 4 GPIO pins as per the [rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix) library instructions.

5. **Configuration**:

- Create a config file named ws.cfg in the root directory with the following contents:
    ```bash
    API_Token=<FinnHub API Key>

    # The values will be accumulated so all of the symbols will be subscribed for
    Symbol_List=GOOGL
    Symbol_List=NVDA
    Symbol_List=BINANCE:BTCUSDT
    Symbol_List=SPY
    Symbol_List=IC MARKETS:1
    Symbol_List=IC MARKETS:2

    # Mapping from the symbol name to the logo name on https://financialmodelingprep.com/image-stock/<logo>.png
    Icon_Mapping=IC MARKETS:1 -> EURUSD
    Icon_Mapping=IC MARKETS:2 -> GBPUSD
    Icon_Mapping=BINANCE:BTCUSDT -> BTCUSD

    Logo_Size=23
    ...
    # See Config.cpp to find out about more config options

6. Build and Run:

   The project follows Core - App architecture.

    ```bash
    make
    ./Binaries/<OS>/Debug/App/App

## Sources
- [FinnHub API](https://finnhub.io/): Provides real-time market prices.
- [rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix): Library for interfacing with the LED board.

## License
- MIT License
- Premake: Licensed under BSD 3-Clause (see included LICENSE.txt for more details).

Enjoy your real-time LED stock ticker display!
