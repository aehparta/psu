
# Microcurrent

MCP3561 based implementation of logging current meter with a range from 67 mA to 0.5 µA.

## Setup

### InfluxDB

#### Install

```sh
apt install influxdb
```

#### Configure UDP

Edit ```/etc/influxdb/influxdb.conf```:

```
[[udp]]
  enabled = true
  bind-address = ":8089"
  database = "microcurrent"
  # retention-policy = ""
  precision = "n"
  batch-size = 5000
  batch-pending = 10
  batch-timeout = "0.5s"
  read-buffer = 0
```
