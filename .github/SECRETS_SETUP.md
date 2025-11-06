# 🔐 GitHub Actions Secrets Setup

Bu dosya BabaChain cross-platform CI/CD pipeline'ının çalışması için gerekli GitHub secrets'larını açıklar.

## 📋 Gerekli Secrets

### 🐳 Docker Hub (Zorunlu)
Docker image'larını yayınlamak için:

```
DOCKER_USERNAME=your_dockerhub_username
DOCKER_PASSWORD=your_dockerhub_password_or_token
```

**Kurulum:**
1. [Docker Hub](https://hub.docker.com) hesabınıza giriş yapın
2. Account Settings > Security > Access Tokens
3. "New Access Token" oluşturun
4. GitHub Repository > Settings > Secrets and variables > Actions
5. `DOCKER_USERNAME` ve `DOCKER_PASSWORD` ekleyin

### 📢 Discord Notifications (Opsiyonel)
Release bildirimlerini Discord'a göndermek için:

```
DISCORD_WEBHOOK_URL=https://discord.com/api/webhooks/YOUR_WEBHOOK_URL
```

**Kurulum:**
1. Discord sunucunuzda bir kanal seçin
2. Kanal ayarları > Integrations > Webhooks
3. "New Webhook" oluşturun
4. Webhook URL'sini kopyalayın
5. GitHub'da `DISCORD_WEBHOOK_URL` secret'ı ekleyin

### 📧 Telegram Notifications (Opsiyonel)
Release bildirimlerini Telegram'a göndermek için:

```
TELEGRAM_BOT_TOKEN=your_bot_token
TELEGRAM_CHAT_ID=your_chat_id
```

**Kurulum:**
1. [@BotFather](https://t.me/botfather) ile bot oluşturun
2. Bot token'ını alın
3. Botunuzu grubunuza/kanalınıza ekleyin
4. Chat ID'yi öğrenmek için: `https://api.telegram.org/bot<TOKEN>/getUpdates`
5. GitHub'da her iki secret'ı da ekleyin

### 🌐 Website Deploy (Opsiyonel)
Website'i otomatik güncellemek için:

```
WEBSITE_DEPLOY_KEY=github_personal_access_token
```

**Kurulum:**
1. GitHub > Settings > Developer settings > Personal access tokens
2. "Generate new token (classic)" 
3. `repo` scope'unu seçin
4. Token'ı GitHub'da `WEBSITE_DEPLOY_KEY` olarak ekleyin

## 🔧 Secrets Kurulum Adımları

### 1. GitHub Repository'ye Git
```
https://github.com/Baba-Chain/BabaChain/settings/secrets/actions
```

### 2. Her Secret İçin:
1. "New repository secret" tıklayın
2. Secret adını girin (yukarıdaki listeden)
3. Secret değerini girin
4. "Add secret" tıklayın

### 3. Kurulumu Test Et
```bash
# Manuel workflow tetikle
gh workflow run release-cross-platform.yml \
  --ref main \
  -f version=v1.0.0-test \
  -f create_release=false
```

## 🧪 Test Secrets

Development/test amaçlı kullanabileceğiniz örnek değerler:

```bash
# Test Docker Hub (gerçek hesap gerekli)
DOCKER_USERNAME=babachaintest
DOCKER_PASSWORD=test_token_here

# Test Discord Webhook (opsiyonel)
DISCORD_WEBHOOK_URL=https://discord.com/api/webhooks/test

# Test Telegram (opsiyonel)  
TELEGRAM_BOT_TOKEN=123456:ABC-DEF1234ghIkl-zyx57W2v1u123ew11
TELEGRAM_CHAT_ID=-1001234567890
```

## 🔍 Secrets Doğrulama

### GitHub Actions Logs
Workflow çalıştırıldığında logları kontrol edin:

```bash
# Başarılı Docker login
✅ Login Succeeded

# Başarılı Discord notification
✅ Discord notification sent

# Başarılı Telegram notification  
✅ Telegram notification sent
```

### Hata Durumları
```bash
# Docker login hatası
❌ Error: Cannot perform an interactive login from a non TTY device

# Discord webhook hatası
❌ Error: Invalid webhook URL

# Telegram bot hatası
❌ Error: Unauthorized
```

## 🛡️ Güvenlik Best Practices

### 1. Minimum Permissions
- Docker token'ları için sadece gerekli repository'lere erişim
- GitHub token'ları için minimum scope kullanın
- Bot token'ları için sadece gerekli permissions

### 2. Token Rotation
```bash
# Her 90 günde bir token'ları yenileyin
# Eski token'ları devre dışı bırakın
# Yeni token'ları GitHub secrets'ta güncelleyin
```

### 3. Monitoring
```bash
# Token kullanımını izleyin
# Anormal aktivite için logları kontrol edin
# Düzenli güvenlik audit'i yapın
```

## 🚨 Sorun Giderme

### Docker Hub Issues
```bash
# Token permissions kontrol et
docker login --username $DOCKER_USERNAME --password $DOCKER_PASSWORD

# Repository erişimi kontrol et
docker push babachain/test:latest
```

### Discord Issues
```bash
# Webhook URL test et
curl -X POST "$DISCORD_WEBHOOK_URL" \
  -H "Content-Type: application/json" \
  -d '{"content": "Test message"}'
```

### Telegram Issues
```bash
# Bot token test et
curl "https://api.telegram.org/bot$TELEGRAM_BOT_TOKEN/getMe"

# Chat ID test et
curl "https://api.telegram.org/bot$TELEGRAM_BOT_TOKEN/sendMessage" \
  -d "chat_id=$TELEGRAM_CHAT_ID&text=Test message"
```

## 📞 Destek

Secrets kurulumunda sorun yaşıyorsanız:

1. **GitHub Issues**: https://github.com/Baba-Chain/BabaChain/issues
2. **Discord**: https://discord.gg/babachain  
3. **Telegram**: https://t.me/babachainofficial

---

**🔐 Güvenlik Uyarısı**: Secret değerlerini asla public repository'lerde, commit mesajlarında veya log dosyalarında paylaşmayın!
