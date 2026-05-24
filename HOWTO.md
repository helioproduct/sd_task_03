
## Запуск проекта

```bash
docker compose up --build -d
```

Подождите 20-30 секунд, пока соберётся образ и сервис начнёт отвечать на `ping`.

## Проверка статуса

```bash
docker compose ps
```

Сервис `mail-lab` должен быть в статусе `running` или `healthy`.

## Тестирование

Запуск всех тестов:

```bash
docker compose exec mail-lab pytest tests -v
```

## Swagger UI

Откройте в браузере: http://localhost:8080/swagger

## Остановка

```bash
docker compose down
```
