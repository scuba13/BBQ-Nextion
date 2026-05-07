"""
Fixtures compartilhadas para a suite de testes da API BBQ-Nextion.

Uso:
    pytest tests/ --host 192.168.1.100
    pytest tests/ --host 192.168.1.100 --key minha-chave
    BBQ_HOST=192.168.1.100 pytest tests/
"""
import os
import pytest
import requests


def pytest_addoption(parser):
    parser.addoption(
        "--host",
        default=os.getenv("BBQ_HOST", "192.168.1.100"),
        help="IP ou hostname do dispositivo BBQ-Nextion",
    )
    parser.addoption(
        "--port",
        default=os.getenv("BBQ_PORT", "80"),
        help="Porta HTTP do dispositivo (padrão: 80)",
    )
    parser.addoption(
        "--key",
        default=os.getenv("BBQ_API_KEY", ""),
        help="API key (X-API-Key) se autenticação estiver ativada",
    )


@pytest.fixture(scope="session")
def base_url(request):
    host = request.config.getoption("--host")
    port = request.config.getoption("--port")
    return f"http://{host}:{port}/api/v1"


@pytest.fixture(scope="session")
def api_key(request):
    return request.config.getoption("--key")


@pytest.fixture(scope="session")
def auth_headers(api_key):
    if api_key:
        return {"X-API-Key": api_key}
    return {}


@pytest.fixture(scope="session")
def session(auth_headers):
    """Sessão requests com headers de auth e timeout padrão."""
    s = requests.Session()
    s.headers.update(auth_headers)
    s.request = lambda method, url, **kwargs: requests.Session.request(
        s, method, url, timeout=kwargs.pop("timeout", 10), **kwargs
    )
    return s


@pytest.fixture(scope="session", autouse=True)
def check_device_reachable(base_url):
    """Aborta toda a suite se o dispositivo não responder."""
    try:
        r = requests.get(f"{base_url}/monitor", timeout=5)
        assert r.status_code == 200, f"Monitor retornou {r.status_code}"
    except requests.exceptions.ConnectionError:
        pytest.exit(
            f"Dispositivo não alcançável em {base_url}. "
            "Use --host <IP> para especificar o endereço.",
            returncode=1,
        )


def get_data(response):
    """Extrai o campo 'data' do envelope de resposta da API."""
    body = response.json()
    return body.get("data", body)
