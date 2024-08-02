# LED Stock Ticker on Raspberry Pi

This project replicates the functionality of real-time LED Stock Ticker displays commonly found in marketplaces like [Etsy Led Stock Ticker](https://www.etsy.com/uk/market/led_stock_ticker).

![nvda](https://github.com/user-attachments/assets/5607ae33-e602-418f-be7a-fbd2ed3eac9c)
![btc](https://github.com/user-attachments/assets/e4434b13-45bf-4955-840d-41fdbb119f1a)


## Features

- Developed a real-time LED stock ticker using C++, PostgreSQL, and Raspberry Pi.
- Utilized WebSockets to interact with financial data APIs for real-time price updates.
- Applied observer design pattern to control the ticker with WebSocket connection to Spring Boot back-end.
- Supports stocks, ETFs, crypto, forex, indices, and commodities subscriptions.
- Designed a multithreading system for efficient data processing and smooth display updates.
- Created visualization price charts with 1-minute intervals and past data stored in a PostgreSQL database.
- Implemented rendering of logos, symbol names, and daily percentage gain/loss on a 64x32 RGB LED display.
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

Don't forget to install the necessary [Adafruit-Hat drivers](https://learn.adafruit.com/adafruit-rgb-matrix-plus-real-time-clock-hat-for-raspberry-pi/driving-matrices).

5. **Configuration**:

- Create a config file named config.cfg in the root directory with the following contents:
    ```bash
    API_Token=<FinnHub API Key>
    Control_API_Token=<API Key obtained from stock-ticker-remote.link> (optional)

    # This doesn't have to be configured if Control_API_Token is specified.
    Subs_list= BTC NVDA 
    Api_Subs_list = COINBASE:BTC-USD NVDA
    Logo_Subs_list = BTCUSD NVDA

    Logo_Size=23

    # Determines whether to display logos or instead display a full 64-column price chart.
    Render_Logos=true
    ...
    # See Config.cpp to find out about more config options

6. Build and Run:

   The project follows Core - App architecture.

    ```bash
    make
    ./Binaries/<OS>/Debug/App/App

## Video

https://github.com/user-attachments/assets/3c610362-67a1-4644-8a2b-8b2a6e6fb934


## Prototype

![Prototype](https://github.com/user-attachments/assets/45b43189-f218-42c4-bcec-dc8e10bd6f71)


## Sources
- [FinnHub API](https://finnhub.io/): Provides real-time market prices.
- [rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix): Library for interfacing with the LED board.

## License
- MIT License
- Premake: Licensed under BSD 3-Clause (see included LICENSE.txt for more details).

Enjoy your real-time LED stock ticker display!
