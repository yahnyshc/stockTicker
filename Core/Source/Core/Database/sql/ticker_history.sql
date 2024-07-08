-- create ticker history table
CREATE TABLE ticker_history (
  symbol VARCHAR(100),
  price FLOAT,
  time TIMESTAMP DEFAULT NOW()
);