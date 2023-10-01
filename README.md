
## Debian
Currently only arm64 (aarch64) is supported. Tested on Ubuntu 20.x.

**add Reptoid repo**
```bash
curl -fsSL https://deb.reptoid.com/gpg-key \
    | sudo gpg --dearmor -o /usr/share/keyrings/reptoid.gpg
echo "deb [signed-by=/usr/share/keyrings/reptoid.gpg] https://deb.reptoid.com focal main" \
    | sudo tee /etc/apt/sources.list.d/reptoid.list

sudo apt update
```

**Install viperfish**
```bash
sudo apt install viperfish
```

## Building from source

```
sudo apt install -y \
    libssl-dev libcurl4 libcurl4-openssl-dev \
    libboost-dev libboost-filesystem-dev libboost-thread-dev
```


```bash
cmake .
make viperfish
make install
```
