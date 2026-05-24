## Запуск проекта

```bash
docker compose up --build -d
```

Сначала поднимется `postgres`, затем API-сервис `mail-lab`.

## Проверка статуса

```bash
docker compose ps
```

Оба контейнера должны быть в статусе `healthy`.

## Тестирование

Запуск всех тестов:

```bash
docker compose exec mail-lab pytest tests -v
```

Проверка SQL-данных:

```bash
docker compose exec postgres psql -U mail_user -d mail_lab_db -c "\dt"
docker compose exec postgres psql -U mail_user -d mail_lab_db -c "select count(*) from users;"
```

## Swagger UI

Откройте в браузере: http://localhost:8080/swagger

## Остановка

```bash
docker compose down
```
