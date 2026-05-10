# Ryazha-Status-Monitor API Documentation

## Overview

Ryazha-Status-Monitor предоставляет API для доступа к данным мониторинга системы Nintendo Switch через различные интерфейсы.

## API Endpoints

### 1. System Information

#### GET `/api/v1/system`
Получение общей информации о системе

**Response:**
```json
{
  "system": {
    "firmware_version": "17.0.1",
    "serial_number": "XAWXXXXXXXX",
    "device_model": "Switch",
    "battery_percentage": 85.5,
    "charging": true
  }
}
```

### 2. CPU Monitoring

#### GET `/api/v1/cpu`
Получение данных о загрузке CPU

**Response:**
```json
{
  "cpu": {
    "cores": [
      {
        "id": 0,
        "usage": 45.2,
        "frequency": 1020000000,
        "temperature": 45.5
      },
      {
        "id": 1,
        "usage": 38.7,
        "frequency": 1020000000,
        "temperature": 44.8
      },
      {
        "id": 2,
        "usage": 42.1,
        "frequency": 1020000000,
        "temperature": 45.2
      },
      {
        "id": 3,
        "usage": 15.3,
        "frequency": 1224000000,
        "temperature": 46.1
      }
    ],
    "total_usage": 35.3
  }
}
```

### 3. GPU Monitoring

#### GET `/api/v1/gpu`
Получение данных о загрузке GPU

**Response:**
```json
{
  "gpu": {
    "usage": 67.8,
    "frequency": 768000000,
    "temperature": 48.5,
    "memory_usage": 1024,
    "memory_total": 4096
  }
}
```

### 4. Memory Monitoring

#### GET `/api/v1/memory`
Получение данных об использовании памяти

**Response:**
```json
{
  "memory": {
    "total": 4194304,
    "used": 2097152,
    "free": 2097152,
    "application": 1048576,
    "applet": 524288,
    "system": 524288,
    "system_unsafe": 0
  }
}
```

### 5. Temperature Monitoring

#### GET `/api/v1/temperature`
Получение данных о температуре

**Response:**
```json
{
  "temperatures": {
    "soc": 48.5,
    "pcb": 45.2,
    "skin": 42.8,
    "battery": 35.5
  }
}
```

### 6. Battery Monitoring

#### GET `/api/v1/battery`
Получение данных о батарее

**Response:**
```json
{
  "battery": {
    "percentage": 85.5,
    "raw_charge": 3584,
    "temperature": 35.5,
    "voltage": 3.85,
    "current": 0.5,
    "power": 1.925,
    "age": 100,
    "charging": true,
    "charger_type": "official",
    "max_voltage": 4.2,
    "max_current": 2.0
  }
}
```

### 7. Network Monitoring

#### GET `/api/v1/network`
Получение данных о сети

**Response:**
```json
{
  "network": {
    "type": "wifi",
    "connected": true,
    "ssid": "MyWiFiNetwork",
    "signal_strength": -45,
    "ip_address": "192.168.1.100",
    "mac_address": "AA:BB:CC:DD:EE:FF"
  }
}
```

### 8. Performance Metrics

#### GET `/api/v1/performance`
Получение метрик производительности

**Response:**
```json
{
  "performance": {
    "fps": 60,
    "pfps": 59.8,
    "resolution": {
      "width": 1920,
      "height": 1080
    },
    "game_read_speed": 25.5,
    "fan_speed": 45
  }
}
```

## WebSocket API

### Real-time Updates

Подключитесь к WebSocket для получения real-time данных:

```javascript
const ws = new WebSocket('ws://localhost:8080/ws');

ws.onmessage = function(event) {
  const data = JSON.parse(event.data);
  console.log('Real-time data:', data);
};
```

**WebSocket Message Format:**
```json
{
  "type": "system_update",
  "timestamp": 1654321000,
  "data": {
    "cpu_usage": 45.2,
    "gpu_usage": 67.8,
    "temperature": 48.5
  }
}
```

## Error Handling

### Error Response Format
```json
{
  "error": {
    "code": 404,
    "message": "Endpoint not found",
    "details": "The requested API endpoint does not exist"
  }
}
```

### Common Error Codes
- `400` - Bad Request
- `404` - Not Found
- `500` - Internal Server Error
- `503` - Service Unavailable

## Rate Limiting

API ограничен до 100 запросов в минуту для одного клиента.

## Authentication

Для доступа к API требуется API ключ:

```http
Authorization: Bearer YOUR_API_KEY
```

## SDK Examples

### Python SDK
```python
import ryazha_api

# Initialize client
client = ryazha_api.Client(api_key="YOUR_API_KEY")

# Get CPU data
cpu_data = client.get_cpu()
print(f"CPU Usage: {cpu_data.total_usage}%")

# Real-time monitoring
def on_update(data):
    print(f"Temperature: {data.temperatures.soc}°C")

client.start_realtime_monitoring(callback=on_update)
```

### JavaScript SDK
```javascript
import { RyazhaAPI } from 'ryazha-api';

// Initialize client
const client = new RyazhaAPI('YOUR_API_KEY');

// Get system data
const system = await client.getSystem();
console.log(`Battery: ${system.battery_percentage}%`);

// Real-time updates
client.on('update', (data) => {
    console.log('Real-time data:', data);
});
```

## Configuration

### Environment Variables
- `RYAZHA_API_PORT` - Порт API сервера (по умолчанию: 8080)
- `RYAZHA_API_KEY` - API ключ для аутентификации
- `RYAZHA_LOG_LEVEL` - Уровень логирования (DEBUG, INFO, WARN, ERROR)

### Configuration File
```json
{
  "api": {
    "port": 8080,
    "enable_auth": true,
    "rate_limit": 100
  },
  "monitoring": {
    "update_interval": 1000,
    "enable_websocket": true
  }
}
```

## Troubleshooting

### Common Issues

1. **API не отвечает**
   - Проверьте, что Ryazha-Status-Monitor запущен
   - Убедитесь, что порт 8080 не заблокирован

2. **WebSocket соединение разрывается**
   - Проверьте сетевое соединение
   - Увеличьте таймаут соединения

3. **Неправильные данные**
   - Убедитесь, что SaltyNX установлен
   - Проверьте совместимость с версией прошивки

## Version History

- **v1.0.0** - Initial API release
- **v1.1.0** - Added WebSocket support
- **v1.2.0** - Enhanced error handling
- **v1.3.0** - Added performance metrics
