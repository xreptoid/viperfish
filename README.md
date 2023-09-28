
### Add GPG key
```bash
curl -fsSL https://reptoid-market-data-00.s3.ap-northeast-1.amazonaws.com/debian-repo/gpg-key | sudo gpg --dearmor -o /usr/share/keyrings/viperfish.gpg
```


### Add Reptoid repo
```bash
echo "deb [signed-by=/usr/share/keyrings/viperfish.gpg] https://reptoid-market-data-00.s3.ap-northeast-1.amazonaws.com/debian-repo focal main" | sudo tee /etc/apt/sources.list.d/viperfish.list

sudo apt update
```
