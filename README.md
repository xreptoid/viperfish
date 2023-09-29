
### Add Reptoid repo
```bash
curl -fsSL https://deb.reptoid.com/gpg-key \
    | sudo gpg --dearmor -o /usr/share/keyrings/reptoid.gpg
echo "deb [signed-by=/usr/share/keyrings/reptoid.gpg] https://deb.reptoid.com focal main" \
    | sudo tee /etc/apt/sources.list.d/reptoid.list

sudo apt update
```

### Install viperfish
```bash
sudo apt install viperfish
```
