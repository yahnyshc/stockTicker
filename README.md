# LED Stock Ticker on Raspberry Pi

This project is a DIY real-time LED stock ticker display, inspired by similar products available on platforms like [Etsy](https://www.etsy.com/uk/market/led_stock_ticker). The project leverages a Raspberry Pi, a 64x32 RGB LED matrix, and financial data APIs to deliver up-to-the-minute price updates for various assets, including stocks, cryptocurrencies, ETFs, forex pairs, indices, and commodities.

![NVDA](https://github.com/user-attachments/assets/9d01dc58-78ed-4307-bb92-516d0bceb419)
![BTC](https://github.com/user-attachments/assets/b91ffbd6-f388-45c5-8e51-bffc0ba75438)
![GBPUSD](https://github.com/user-attachments/assets/ca922ec2-4a53-4412-9ee2-b2dff9759964)
![NDAQ](https://github.com/user-attachments/assets/63d27c2a-dc29-4f24-963d-16f899e2aa21)

## Video

![Demo](https://github.com/user-attachments/assets/590db6de-6775-465e-a527-f10d186da9fb)

## Overview
The LED stock ticker display is designed to visually present real-time financial data on a bright and colorful RGB LED matrix. It’s powered by a Raspberry Pi and features a multi-threaded system for efficient data handling and smooth display updates. The system is capable of rendering asset logos, symbol names, daily percentage changes, and mini-price charts.

## Features

- **Real-Time Data Updates**: Utilizes WebSockets to fetch live data from financial APIs like FinnHub.
- **Support for Multiple Asset Classes**: Display prices and charts for stocks, ETFs, cryptocurrencies, forex pairs, indices, and commodities.
- **Customizable Display**: Choose between displaying asset logos with price info or full 64-column price charts.
- **Multithreading**: Ensures smooth and responsive operation, even with multiple data streams.
- **Data Storage**: Price data is stored in a PostgreSQL database, enabling historical analysis and charting.
- **Web-Based Control**: Configure the ticker, manage API keys, and customize subscriptions via a web interface.

## Hardware Requirements

- Raspberry Pi 4B
- Adafruit 64x32 RGB Matrix
- Adafruit Bonnet
- Power Supply

## Configure using website

To control the configuration, obtain the API key and set up subscriptions, logos, and API names using [stock-ticker-remote](https://stock-ticker-remote.link)

[Stock Ticker Remote Github](https://github.com/yahnyshc/stockTickerRemote)

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

6. **Build and Run**:

   The project follows Core - App architecture.

    ```bash
    make
    ./Binaries/<OS>/Debug/App/App

## Prototype

![Prototype](https://github.com/user-attachments/assets/45b43189-f218-42c4-bcec-dc8e10bd6f71)


## Sources
- [FinnHub API](https://finnhub.io/): Provides real-time market prices.
- [rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix): Library for interfacing with the LED board.

## License
- MIT License
- Premake: Licensed under BSD 3-Clause (see included LICENSE.txt for more details).

With this setup, you can build a custom, real-time stock ticker display that not only enhances your space but keeps you updated with the latest market trends. Enjoy creating your personalized LED stock ticker!
