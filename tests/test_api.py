import os
import time
import uuid

import requests


SERVICE_URL = os.getenv("SERVICE_URL", "http://127.0.0.1:8080")


def unique_login():
    return f"user_{int(time.time() * 1000)}_{uuid.uuid4().hex[:6]}"


def unique_email():
    return f"mail_{int(time.time() * 1000)}_{uuid.uuid4().hex[:6]}@example.com"


def create_user_payload(first_name="Test", last_name="User"):
    return {
        "login": unique_login(),
        "email": unique_email(),
        "first_name": first_name,
        "last_name": last_name,
        "password": "password123",
    }


def post(path, payload=None, headers=None):
    return requests.post(f"{SERVICE_URL}{path}", json=payload, headers=headers or {}, timeout=10)


def get(path, headers=None):
    return requests.get(f"{SERVICE_URL}{path}", headers=headers or {}, timeout=10)


def login_headers(user_data):
    response = post("/api/v1/auth/login", {"login": user_data["login"], "password": user_data["password"]})
    token = response.json()["token"]
    return {"Authorization": f"Bearer {token}"}


def test_ping():
    response = get("/ping")
    assert response.status_code == 200
    assert response.text == "pong\n"


def test_full_user_flow():
    user_data = create_user_payload("Anna", "Stone")
    created = post("/api/v1/users", user_data)
    assert created.status_code == 201
    assert "password" not in created.json()

    headers = login_headers(user_data)
    lookup = get(f"/api/v1/users/by-login?login={user_data['login']}", headers=headers)
    search = get("/api/v1/users/search?mask=Anna", headers=headers)

    assert lookup.status_code == 200
    assert lookup.json()["login"] == user_data["login"]
    assert any(item["login"] == user_data["login"] for item in search.json())


def test_user_validation_and_duplicates():
    broken = post("/api/v1/users", {"login": "", "email": "bad", "first_name": "", "last_name": "", "password": ""})
    assert broken.status_code == 400

    user = create_user_payload()
    assert post("/api/v1/users", user).status_code == 201
    assert post("/api/v1/users", {**create_user_payload(), "login": user["login"]}).status_code == 409
    assert post("/api/v1/users", {**create_user_payload(), "email": user["email"]}).status_code == 409


def test_folder_and_message_flow():
    first_user = create_user_payload("First", "Owner")
    second_user = create_user_payload("Second", "Owner")
    post("/api/v1/users", first_user)
    post("/api/v1/users", second_user)

    first_headers = login_headers(first_user)
    second_headers = login_headers(second_user)

    folder_response = post("/api/v1/folders", {"name": "Inbox"}, headers=first_headers)
    folder_id = folder_response.json()["id"]

    message_response = post(
        f"/api/v1/folders/{folder_id}/messages",
        {"subject": "Hello", "body": "Body text", "recipient_email": "to@example.com"},
        headers=first_headers,
    )
    message_id = message_response.json()["id"]

    listed = get(f"/api/v1/folders/{folder_id}/messages", headers=first_headers)
    fetched = get(f"/api/v1/messages/{message_id}", headers=first_headers)
    foreign = get(f"/api/v1/messages/{message_id}", headers=second_headers)

    assert folder_response.status_code == 201
    assert message_response.status_code == 201
    assert listed.status_code == 200
    assert fetched.status_code == 200
    assert fetched.json()["subject"] == "Hello"
    assert foreign.status_code == 403


def test_auth_and_validation_errors():
    no_auth_folders = get("/api/v1/folders")
    invalid_token = get("/api/v1/folders", headers={"Authorization": "Bearer bad-token"})

    assert no_auth_folders.status_code == 400
    assert invalid_token.status_code == 403

    user_data = create_user_payload()
    post("/api/v1/users", user_data)
    headers = login_headers(user_data)
    folder_id = post("/api/v1/folders", {"name": "Drafts"}, headers=headers).json()["id"]

    bad_message = post(
        f"/api/v1/folders/{folder_id}/messages",
        {"subject": "", "body": "Body", "recipient_email": "wrong"},
        headers=headers,
    )
    invalid_folder_id = get("/api/v1/folders/not-a-number/messages", headers=headers)
    invalid_message_id = get("/api/v1/messages/not-a-number", headers=headers)

    assert bad_message.status_code == 400
    assert invalid_folder_id.status_code == 400
    assert invalid_message_id.status_code == 400
