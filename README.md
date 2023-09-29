
### Add Reptoid repo
```bash
curl -fsSL https://deb.reptoid.com/gpg-key \
    | sudo gpg --dearmor -o /usr/share/keyrings/viperfish.gpg
echo "deb [signed-by=/usr/share/keyrings/viperfish.gpg] https://deb.reptoid.com focal main" \
    | sudo tee /etc/apt/sources.list.d/viperfish.list

sudo apt update
```

### Install viperfish
```bash
sudo apt install viperfish
```
